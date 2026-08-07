#pragma once

#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/trivial.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace logger {

namespace log = boost::log;
namespace sinks = boost::log::sinks;

struct SourceInfo {
    std::string file;
    std::string function;
    uint32_t line;
};

#define SOURCE_INFO                      \
    SourceInfo {                         \
        __FILE__, __FUNCTION__, __LINE__ \
    }
#define LOG_INFO(message) logger::Logger::LogInfo(logger::SOURCE_INFO, message)
#define LOG_ERROR(message) logger::Logger::LogError(logger::SOURCE_INFO, message)

class Logger {
public:
    static Logger& Instance();

    ~Logger();

    static void LogInfo(const SourceInfo& source_info, const std::string& message);
    static void LogError(const SourceInfo& source_info, const std::string& message);

private:
    Logger();
    static void Initialize();
    static std::string GetLogDirPath();
    static std::string GetProcessIdentifier();

private:
    using async_synk = sinks::asynchronous_sink<sinks::text_file_backend>;
    static std::vector<boost::shared_ptr<async_synk>> sinks_;
};

}  // namespace logger
