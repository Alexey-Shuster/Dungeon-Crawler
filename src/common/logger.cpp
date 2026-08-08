#include "logger.h"

#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/bounded_fifo_queue.hpp>
#include <boost/log/sinks/unbounded_fifo_queue.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "config.h"

namespace logger {

std::vector<boost::shared_ptr<Logger::async_synk>> Logger::sinks_;

namespace keywords = log::keywords;
namespace json = boost::json;

constexpr std::string_view LOG_DIR_PATH = "LOG_DIR_PATH";
constexpr std::string_view DEFAULT_LOG_DIR_PATH = "./logs";
constexpr const uint32_t LOG_SIZE_ROTATION{10 * 1024 * 1024};
const sinks::file::rotation_at_time_point LOG_TIME_ROTATION{0, 0, 0};

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_function, "Function", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_line, "Line", uint32_t)

std::string Logger::GetProcessIdentifier() {
    static std::string process_identifier = []() {
        boost::uuids::random_generator gen;
        boost::uuids::uuid process_id = gen();
        return boost::uuids::to_string(process_id);
    }();
    return process_identifier;
}

std::string Logger::GetLogDirPath() {
    auto config_log_dir_path = config::getSettings().logger.output_dir;

    if (!config_log_dir_path.empty()) {
        std::cout << "[Logger] Info: Using logger.output_dir parameter from config file: " << config_log_dir_path
                  << std::endl;
        return config_log_dir_path;
    } else {
        std::cout << "[Logger] Info: Config file not found, checking environment variable..." << std::endl;
    }

    if (const char* env_log_dir_path = std::getenv(LOG_DIR_PATH.data())) {
        std::cout << "[Logger] Info: Using LOGGER_OUTPUT_DIR from environment variable: " << env_log_dir_path
                  << std::endl;
        return env_log_dir_path;
    } else {
        std::cout << "[Logger] Info: LOGGER_OUTPUT_DIR environment variable not set." << std::endl;
    }

    std::cout << "[Logger] Info: Using default log directory: " << DEFAULT_LOG_DIR_PATH << std::endl;
    return std::string(DEFAULT_LOG_DIR_PATH);
}

void LogFormatter(const log::record_view& rec, log::formatting_ostream& strm) {
    auto severity = *rec[log::trivial::severity];
    auto ts = *rec[timestamp];
    auto file = *rec[source_file];
    auto function = *rec[source_function];
    auto line = *rec[source_line];
    auto message = *rec[log::expressions::smessage];

    json::object log_data;
    log_data["severity"] = boost::log::trivial::to_string(severity);
    log_data["timestamp"] = to_iso_extended_string(ts);
    log_data["file"] = file;
    log_data["function"] = function;
    log_data["line"] = line;
    log_data["message"] = message;

    strm << json::serialize(log_data);
}

void Logger::Initialize() {
    log::add_common_attributes();

    std::string log_dir_path = GetLogDirPath();
    std::string process_identifier = GetProcessIdentifier();

    std::filesystem::create_directories(log_dir_path);

    std::filesystem::path log_file_name =
        std::filesystem::path(log_dir_path) / process_identifier / "%Y-%m-%d_%H-%M-%S_%N.log";

    auto file_backend = boost::make_shared<sinks::text_file_backend>(keywords::file_name = log_file_name.string(),
                                                                     keywords::rotation_size = LOG_SIZE_ROTATION,
                                                                     keywords::time_based_rotation = LOG_TIME_ROTATION,
                                                                     keywords::auto_flush = true);

    auto sink = boost::make_shared<async_synk>(file_backend);
    sink->set_filter(log::trivial::severity >= log::trivial::info);
    sink->set_formatter(&LogFormatter);

    log::core::get()->add_sink(boost::static_pointer_cast<log::sinks::sink>(sink));
    sinks_.push_back(sink);
}

Logger::Logger() {
    try {
        static std::once_flag initialize_flag;
        std::call_once(initialize_flag, Initialize);
    } catch (const std::exception& excp) {
        std::cerr << "[WARNING] Logger initialization failed: " << excp.what() << std::endl;
        throw;
    }
}

Logger& Logger::Instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    try {
        auto core = log::core::get();
        for (auto& sink : sinks_) {
            if (sink) {
                core->remove_sink(sink);
                sink->stop();
                sink->flush();
                sink.reset();
            }
        }
    } catch (const std::exception& excp) {
        std::cerr << "WARNING: exceptions in Logger destructor: " << excp.what();
    }
}

void Logger::LogInfo(const SourceInfo& source_info, const std::string& message) {
    BOOST_LOG_TRIVIAL(info) << log::add_value("File", source_info.file)
                            << log::add_value("Function", source_info.function)
                            << log::add_value("Line", source_info.line) << message;
}

void Logger::LogError(const SourceInfo& source_info, const std::string& message) {
    BOOST_LOG_TRIVIAL(error) << log::add_value("File", source_info.file)
                             << log::add_value("Function", source_info.function)
                             << log::add_value("Line", source_info.line) << message;
}

}  // namespace logger
