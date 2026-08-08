#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "win_defs.h"  // IWYU pragma: keep // windows.h

#ifndef _WIN32
    #include <X11/Xlib.h>
    #include <X11/keysym.h>
#endif

namespace utility {

/**
 * Asynchronous keyboard reader using Boost.Asio timers.
 *
 * Polls the physical keyboard state at a fixed interval and invokes a callback
 * with the corresponding command string (see keyboard_controller::commandFromKey).
 *
 * On Linux, requires X11; link with -lX11. If X11 is unavailable, a warning is
 * printed and the reader does nothing.
 */
class KeyboardReader : public std::enable_shared_from_this<KeyboardReader> {
public:
    using KeyCallback = std::function<void(char key)>;

    /**
     * @param io             Boost.Asio io_context
     * @param edge_triggered If true, callback fires only once per key press
     *                       (on key-down edge). If false (default), fires
     *                       continuously while the key is held down.
     */
    explicit KeyboardReader(boost::asio::io_context& io, bool edge_triggered = false);
    ~KeyboardReader();

    void start(KeyCallback cb, std::chrono::milliseconds interval = std::chrono::milliseconds(20));
    void stop();

protected:
    virtual void fetchKeyState();            // platform‑specific state retrieval
    virtual bool isKeyDown(char key) const;  // uses the cached state

private:
    void poll();

    boost::asio::io_context& io_context_;
    boost::asio::steady_timer timer_;
    KeyCallback callback_;
    std::chrono::milliseconds interval_{};
    const bool edge_triggered_;
    std::unordered_map<char, bool> prev_state_;  // only used if edge_triggered_

    std::atomic<bool> running_{false};
    mutable std::mutex x11_mutex_;  // protects X11 calls if multiple threads

    // Platform‑specific state (cached per poll)
#ifdef _WIN32
    // No extra state needed – GetAsyncKeyState is thread‑safe.
#else
    std::unordered_map<char, KeyCode> key_code_map_;
    char x11_keymap_[32];  // current X11 keymap
    Display* display_ = nullptr;
#endif
};

}  // namespace utility
