#pragma once

#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <mutex>
#include <string>
#include <vector>

namespace utils {

struct LogConfig {
    std::string log_dir = "./logs";
    std::string log_level = "info";
    std::string log_format = "json";
};

struct LogSrcInfo {
    const char* file;
    const char* function;
    uint32_t line;

    // Implicit conversion from C++20 standard source location
    constexpr LogSrcInfo(std::source_location loc = std::source_location::current()) noexcept
        : file(loc.file_name())
        , function(loc.function_name())
        , line(loc.line()) {}
};

#define LOG_INFO(msg) ::utils::Logger::Instance().LogInfo(msg)
#define LOG_ERROR(msg) ::utils::Logger::Instance().LogError(msg)
#define LOG_DEBUG(msg) ::utils::Logger::Instance().LogDebug(msg)
#define LOG_WARN(msg) ::utils::Logger::Instance().LogWarn(msg)

class Logger {
public:
    static Logger& Instance();
    static void Initialize(const LogConfig& config = LogConfig{});
    static void Reset();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void LogInfo(std::string_view message, LogSrcInfo src = LogSrcInfo{});
    void LogError(std::string_view message, LogSrcInfo src = LogSrcInfo{});
    void LogDebug(std::string_view message, LogSrcInfo src = LogSrcInfo{});
    void LogWarn(std::string_view message, LogSrcInfo src = LogSrcInfo{});

private:
    Logger(const LogConfig& config);
    ~Logger();
    friend struct std::default_delete<Logger>;

    void Shutdown();
    static std::string GetProcessIdentifier();

    template <typename LogStream>
    void LogWithSource(LogStream&& stream, const LogSrcInfo& source_info, std::string_view message);

private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex init_mutex_;

    using async_synk = boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>;
    std::vector<boost::shared_ptr<boost::log::sinks::sink>> sinks_{};
};

template <typename LogStream>
void Logger::LogWithSource(LogStream&& stream, const LogSrcInfo& source_info, std::string_view message) {
    stream << boost::log::add_value("File", source_info.file) << boost::log::add_value("Function", source_info.function)
           << boost::log::add_value("Line", source_info.line) << message;
}

}  // namespace utils
