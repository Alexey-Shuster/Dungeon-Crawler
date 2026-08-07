#include "keyboard_reader.h"

#include <array>
#include <cctype>
#include <iostream>

namespace utility {

// Platform‑independent constants for key‑state queries
#ifdef _WIN32
static constexpr int kKeyDownMask = 0x8000;  // most significant bit indicates "pressed"
#endif

// List of keys we monitor (case‑insensitive)
static constexpr std::array<char, 5> kKeys = {'w', 'a', 's', 'd', 'x'};

KeyboardReader::KeyboardReader(boost::asio::io_context& io, bool edge_triggered) :
    io_context_(io), timer_(io), edge_triggered_(edge_triggered) {
#ifndef _WIN32
    // Ensure X11 is thread‑safe
    static bool x11_initialized = []() {
        XInitThreads();
        return true;
    }();
    (void)x11_initialized;

    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        std::cerr << "Warning: XOpenDisplay failed – keyboard reader disabled.\n";
    }
#endif
}

KeyboardReader::~KeyboardReader() {
    stop();
#ifndef _WIN32
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
#endif
}

void KeyboardReader::start(KeyCallback cb, std::chrono::milliseconds interval) {
    if (running_.exchange(true)) {
        // already running – silently ignore
        return;
    }
    callback_ = std::move(cb);
    interval_ = interval;

    if (edge_triggered_) {
        for (char key : kKeys) {
            prev_state_[key] = false;
        }
    }

#ifndef _WIN32
    // Pre‑translate all KeySyms to KeyCodes once.
    if (display_) {
        std::lock_guard<std::mutex> lock(x11_mutex_);
        for (char key : kKeys) {
            char lower = std::tolower(static_cast<unsigned char>(key));
            KeySym ks = 0;
            // Map the character to the corresponding X11 KeySym.
            switch (lower) {
                case 'w':
                    ks = XK_w;
                    break;
                case 'a':
                    ks = XK_a;
                    break;
                case 's':
                    ks = XK_s;
                    break;
                case 'd':
                    ks = XK_d;
                    break;
                case 'x':
                    ks = XK_x;
                    break;
                default:
                    continue;  // should not happen
            }
            KeyCode kc = XKeysymToKeycode(display_, ks);
            if (kc != 0) {
                key_code_map_[lower] = kc;  // store with lower‑case key
            }
        }
    }
#endif

    poll();
}

void KeyboardReader::stop() {
    running_ = false;
    boost::system::error_code ec;
    timer_.cancel(ec);
}

void KeyboardReader::poll() {
    // If stopped, do not reschedule
    if (!running_.load()) {
        return;
    }

    // Fetch the current keyboard state (platform‑specific)
    fetchKeyState();

    // Process each key
    for (char key : kKeys) {
        bool now_down = isKeyDown(key);

        if (edge_triggered_) {
            auto it = prev_state_.find(key);
            bool prev_down = (it != prev_state_.end()) ? it->second : false;
            if (now_down && !prev_down) {
                if (callback_)
                    callback_(key);
            }
            prev_state_[key] = now_down;
        } else {
            if (now_down) {
                if (callback_)
                    callback_(key);
            }
        }
    }

    // Schedule the next poll only if still running
    if (running_.load()) {
        timer_.expires_after(interval_);
        timer_.async_wait([self = shared_from_this()](boost::system::error_code ec) {
            if (ec != boost::asio::error::operation_aborted && self->running_.load()) {
                self->poll();
            }
        });
    }
}

void KeyboardReader::fetchKeyState() {
#ifdef _WIN32
    // GetAsyncKeyState is thread‑safe; no caching needed.
    // just use it directly in isKeyDown
#else
    if (!display_)
        return;
    std::lock_guard<std::mutex> lock(x11_mutex_);
    XQueryKeymap(display_, x11_keymap_);
#endif
}

bool KeyboardReader::isKeyDown(char key) const {
    const char lower = std::tolower(static_cast<unsigned char>(key));

#ifdef _WIN32
    // Use explicit virtual‑key codes
    int vk = 0;
    switch (lower) {
        case 'w':
            vk = 'W';
            break;
        case 'a':
            vk = 'A';
            break;
        case 's':
            vk = 'S';
            break;
        case 'd':
            vk = 'D';
            break;
        case 'x':
            vk = 'X';
            break;
        default:
            return false;
    }
    return (GetAsyncKeyState(vk) & kKeyDownMask) != 0;

#else
    if (!display_)
        return false;

    std::lock_guard<std::mutex> lock(x11_mutex_);
    auto it = key_code_map_.find(lower);
    if (it == key_code_map_.end())
        return false;

    KeyCode kc = it->second;

    // Check the corresponding bit in the cached keymap.
    static constexpr int kByteShift = 3;  // divide keycode by 8
    static constexpr int kBitMask = 7;    // modulo 8
    return (x11_keymap_[kc >> kByteShift] & (1 << (kc & kBitMask))) != 0;
#endif
}

}  // namespace utility
