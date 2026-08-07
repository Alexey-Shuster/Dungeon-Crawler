#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

// Platform-specific includes
#ifdef _WIN32
#include <boost/asio/windows/stream_handle.hpp>
#else
#include <boost/asio/posix/stream_descriptor.hpp>
#endif

namespace utility {

class StdinReader : public std::enable_shared_from_this<StdinReader> {
public:
    using LineCallback = std::function<void(const std::string&)>;

    explicit StdinReader(boost::asio::io_context& io) :
        io_context_(io)
#ifdef _WIN32
        ,
        stdin_handle_(io, openConsoleInputHandle())
#else
        ,
        stdin_descriptor_(io, ::dup(STDIN_FILENO))
#endif
    {
    }

    ~StdinReader() {
        stop();
    }

    // Start reading lines, optionally with a prompt.
    void start(LineCallback cb, const std::string& prompt = "> ") {
        callback_ = std::move(cb);
        prompt_ = prompt;

        doReadLine();
    }

    // Stop reading (cancels pending operations) – called automatically on io_context stop.
    void stop() {
#ifdef _WIN32
        stdin_handle_.cancel();
#else
        stdin_descriptor_.cancel();
#endif
    }

private:
#ifdef _WIN32
    static HANDLE openConsoleInputHandle() {
        HANDLE h =
            ::CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open CONIN$ for overlapped I/O");
        }
        return h;
    }
#endif

    auto& getHandle() {
#ifdef _WIN32
        return stdin_handle_;
#else
        return stdin_descriptor_;
#endif
    }

    void doReadLine() {
        // Prompt for new line
        std::cout << prompt_ << std::flush;

        // Use a member buffer to accumulate data
        boost::asio::async_read_until(getHandle(),
                                      boost::asio::dynamic_buffer(buffer_),
                                      '\n',
                                      [this, self = shared_from_this()](boost::system::error_code ec, size_t length) {
                                          if (ec) {
                                              // Cancel or EOF – just stop
                                              return;
                                          }

                                          // Extract the line (including the newline)
                                          std::string line = buffer_.substr(0, length);
                                          buffer_.erase(0, length);

                                          // Trim trailing newline/CR
                                          if (!line.empty() && line.back() == '\n')
                                              line.pop_back();
                                          if (!line.empty() && line.back() == '\r')
                                              line.pop_back();

                                          // Invoke the callback
                                          if (callback_)
                                              callback_(line);

                                          doReadLine();
                                      });
    }

    boost::asio::io_context& io_context_;

#ifdef _WIN32
    boost::asio::windows::stream_handle stdin_handle_;
#else
    boost::asio::posix::stream_descriptor stdin_descriptor_;
#endif

    std::string buffer_;
    LineCallback callback_;
    std::string prompt_;
};

}  // namespace utility
