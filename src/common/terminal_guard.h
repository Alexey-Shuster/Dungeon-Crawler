#pragma once

#include "win_defs.h"  // IWYU pragma: keep // windows.h

#ifndef _WIN32
    #include <termios.h>
    #include <unistd.h>
#endif

namespace utility {

class TerminalGuard {
public:
    TerminalGuard() {
#ifdef _WIN32
        handle_ = GetStdHandle(STD_INPUT_HANDLE);
        if (handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr) {
            if (GetConsoleMode(handle_, &orig_mode_)) {
                DWORD new_mode = orig_mode_ & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
                SetConsoleMode(handle_, new_mode);
                active_ = true;
            }
        }
#else
        if (tcgetattr(STDIN_FILENO, &orig_termios_) == 0) {
            struct termios new_termios = orig_termios_;
            new_termios.c_lflag &= ~(ECHO | ICANON);
            tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
            active_ = true;
        }
#endif
    }

    ~TerminalGuard() {
        restore();
    }

    TerminalGuard(const TerminalGuard&) = delete;
    TerminalGuard& operator=(const TerminalGuard&) = delete;
    TerminalGuard(TerminalGuard&&) = delete;
    TerminalGuard& operator=(TerminalGuard&&) = delete;

    // Explicitly restore early (e.g., before program exit)
    void restore() {
        if (!active_)
            return;
#ifdef _WIN32
        if (handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr) {
            SetConsoleMode(handle_, orig_mode_);
        }
#else
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
#endif
        active_ = false;
    }

    // Check if the terminal mode was successfully changed
    [[nodiscard]] bool isActive() const {
        return active_;
    }

private:
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    DWORD orig_mode_ = 0;
#else
    struct termios orig_termios_;
#endif
    bool active_ = false;
};

}  // namespace utility
