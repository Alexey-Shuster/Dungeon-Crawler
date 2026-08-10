#pragma once

#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace utils {

struct LogSrcInfo {
    std::string file;
    std::string function;
    uint32_t line;
};

#define LOG_SRC_INFO                     \
    LogSrcInfo {                         \
        __FILE__, __FUNCTION__, __LINE__ \
    }

#define LOG_INFO(message) utils::Logger::LogInfo(utils::LOG_SRC_INFO, message)
#define LOG_ERROR(message) utils::Logger::LogError(utils::LOG_SRC_INFO, message)
#define LOG_DEBUG(message) utils::Logger::LogDebug(utils::LOG_SRC_INFO, message)
#define LOG_WARN(message) utils::Logger::LogWarn(utils::LOG_SRC_INFO, message)

class Logger {
public:
    static Logger& Instance();

    ~Logger();

    static void LogInfo(const LogSrcInfo& source_info, const std::string& message);
    static void LogError(const LogSrcInfo& source_info, const std::string& message);
    static void LogDebug(const LogSrcInfo& source_info, const std::string& message);
    static void LogWarn(const LogSrcInfo& source_info, const std::string& message);

private:
    Logger();
    static void Initialize();
    static std::string GetLogDirPath();
    static std::string GetProcessIdentifier();

    template <typename LogStream>
    void LogWithSource(LogStream&& stream, const LogSrcInfo& source_info, const std::string& message);

private:
    using async_synk = boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>;
    inline static std::vector<boost::shared_ptr<async_synk>> sinks_;
};

template <typename LogStream>
void Logger::LogWithSource(LogStream&& stream, const LogSrcInfo& source_info, const std::string& message) {
    stream << boost::log::add_value("File", source_info.file) << boost::log::add_value("Function", source_info.function)
           << boost::log::add_value("Line", source_info.line) << message;
}

}  // namespace utils
