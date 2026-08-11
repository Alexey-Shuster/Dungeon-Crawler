#include "logger.h"

#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/bounded_fifo_queue.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <system_error>

namespace utils {

namespace {

namespace fs = std::filesystem;

bool IsLogDirValid(const std::string& path_str) {
    if (path_str.empty()) {
        return false;
    }

    fs::path log_dir(path_str);
    std::error_code ec;

    // try to create dir
    if (!fs::exists(log_dir, ec)) {
        fs::create_directories(log_dir, ec);
        if (ec) {
            return false;
        }
    }

    // check if is dir
    if (!fs::is_directory(log_dir, ec) || ec) {
        return false;
    }

    // check write permission
    fs::path test_file = log_dir / ".logger_write_test";
    std::ofstream test_stream(test_file);

    if (!test_stream.is_open()) {
        return false;
    }

    test_stream.close();
    fs::remove(test_file, ec);

    return true;
}
}  // namespace

std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::mutex Logger::init_mutex_;

namespace log = boost::log;
namespace sinks = log::sinks;

namespace keywords = log::keywords;
namespace json = boost::json;

constexpr const uint32_t LOG_SIZE_ROTATION{10 * 1024 * 1024};
const sinks::file::rotation_at_time_point LOG_TIME_ROTATION{0, 0, 0};

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_function, "Function", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(source_line, "Line", uint32_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(pid, "PID", int)
BOOST_LOG_ATTRIBUTE_KEYWORD(tid, "TID", boost::log::attributes::current_thread_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(run_uuid, "RunUUID", std::string)

class CustomLogFormatter {
public:
    explicit CustomLogFormatter(std::string format)
        : format_(std::move(format)) {}

    void operator()(const boost::log::record_view& rec, boost::log::formatting_ostream& strm) const {
        auto severity = *rec[boost::log::trivial::severity];
        auto ts = *rec[timestamp];
        auto file = *rec[source_file];
        auto function = *rec[source_function];
        auto line = *rec[source_line];
        auto process_id = *rec[pid];
        auto thread_id_str = boost::lexical_cast<std::string>(*rec[tid]);
        auto run_uuid_val = *rec[run_uuid];
        auto message = *rec[boost::log::expressions::smessage];

        if (format_ == "json") {
            json::object log_data;
            log_data["timestamp"] = to_iso_extended_string(ts);
            log_data["severity"] = boost::log::trivial::to_string(severity);
            log_data["pid"] = process_id;
            log_data["tid"] = thread_id_str;
            log_data["run_uuid"] = run_uuid_val;
            log_data["file"] = file;
            log_data["line"] = line;
            log_data["function"] = function;
            log_data["message"] = message;
            strm << json::serialize(log_data);
        } else {
            // format: [timestamp] [severity] [pid:tid] [run_uuid] [file:line] [function] message
            strm << "[" << to_iso_extended_string(ts) << "]"
                 << " [" << boost::log::trivial::to_string(severity) << "]"
                 << " [pid:tid = " << process_id << ":" << thread_id_str << "]"
                 << " [uuid = " << run_uuid_val << "]"
                 << " [" << file << " : " << line << "]"
                 << " [" << function << "] " << message;
        }
    }

private:
    std::string format_;
};

Logger& Logger::Instance() {
    // Double-Checked Locking Pattern (DCLP)
    if (!instance_) {
        std::lock_guard lock(init_mutex_);
        if (!instance_) {
            std::clog << "[Logger] Warning: Instance() called prior to Initialize(). Default log config used."
                      << std::endl;
            instance_.reset(new Logger(LogConfig{}));
        }
    }
    return *instance_;
}

void Logger::Initialize(const LogConfig& config) {
    std::lock_guard lock(init_mutex_);
    if (!instance_) {
        instance_.reset(new Logger(config));
    } else {
        std::clog << "[Logger] Warning: Logger is already initialized. Call ignored." << std::endl;
    }
}

void Logger::Reset() {
    std::lock_guard lock(init_mutex_);
    if (instance_) {
        instance_->Shutdown();  // stop sinks
        instance_.reset();      // delete logger object
    }
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Shutdown() {
    static bool already_shutdown = false;
    if (already_shutdown)
        return;

    try {
        auto core = log::core::get();
        core->flush();
        for (auto& sink : sinks_) {
            if (sink) {
                core->remove_sink(sink);
                sink.reset();  // auto-stop sinks while destruction
            }
        }
        sinks_.clear();
        already_shutdown = true;
    } catch (const std::exception& e) {
        std::cerr << "[Logger] Warning: exceptions in Logger::Shutdown: " << e.what() << std::endl;
    }
}

Logger::Logger(const LogConfig& config) {
    namespace fs = std::filesystem;

    std::string validated_log_dir = config.log_dir;
    bool use_file_logging = true;

    if (!IsLogDirValid(validated_log_dir)) {
        std::cerr << "[Logger] Warning: Invalid or unwritable log directory: " + config.log_dir << std::endl;
        auto default_log_dir = LogConfig{}.log_dir;
        std::clog << "[Logger] Info: Trying default directory: " << default_log_dir << std::endl;

        if (IsLogDirValid(default_log_dir)) {
            validated_log_dir = default_log_dir;
        } else {
            std::cerr << "[Logger] Error: Default log directory also invalid: " << default_log_dir << std::endl;
            std::clog << "[Logger] Info: Falling back to console-only output." << std::endl;
            use_file_logging = false;
        }
    }

    log::add_common_attributes();

    std::string log_format = config.log_format;
    if (log_format == "json") {
        std::clog << "[Logger] Info: Using JSON log format." << std::endl;
    } else if (log_format == "text") {
        std::clog << "[Logger] Info: Using text log format." << std::endl;
    } else {
        log_format = LogConfig{}.log_format;
        std::clog << "[Logger] Warning: Unknown log format '" << config.log_format
                  << "'. Using default format: " << log_format << std::endl;
    }

    // debug (0) < info (1) < warning (2) < error (3) < fatal (4)

    auto main_log_level = log::trivial::info;
    auto config_log_level = config.log_level;
    std::ranges::transform(config_log_level, config_log_level.begin(), ::tolower);

    if (config_log_level == "warn") {
        main_log_level = log::trivial::warning;
    } else if (config_log_level == "error") {
        main_log_level = log::trivial::error;
    }

    log::core::get()->add_global_attribute("PID", boost::log::attributes::make_constant(::getpid()));
    log::core::get()->add_global_attribute("TID", boost::log::attributes::current_thread_id());
    log::core::get()->add_global_attribute("RunUUID", boost::log::attributes::make_constant(GetProcessIdentifier()));

    if (use_file_logging) {
        fs::path absolute_log_path = fs::absolute(validated_log_dir);

        std::string process_identifier = GetProcessIdentifier();
        std::filesystem::create_directories(absolute_log_path);

        // Base file name pattern
        auto base_path = absolute_log_path / process_identifier;
        std::string base_pattern = (base_path / "%Y-%m-%d_%H-%M-%S_%N").string();

        // Helper to create and add a sink
        auto create_file_sink = [&](const std::string& suffix, log::trivial::severity_level min_sev) {
            std::string file_pattern = base_pattern + suffix + ".log";
            auto backend =
                boost::make_shared<sinks::text_file_backend>(keywords::file_name = file_pattern,
                                                             keywords::rotation_size = LOG_SIZE_ROTATION,
                                                             keywords::time_based_rotation = LOG_TIME_ROTATION,
                                                             keywords::auto_flush = true);

            auto sink = boost::make_shared<async_synk>(backend);
            sink->set_filter(log::trivial::severity >= min_sev);
            sink->set_formatter(CustomLogFormatter(log_format));

            sinks_.push_back(sink);
            log::core::get()->add_sink(sink);
        };

        // Main log – always INFO and above
        create_file_sink("", main_log_level);

        // Debug log – only when config level is "debug"
        if (config_log_level == "debug") {
            create_file_sink("_debug", log::trivial::debug);
            std::clog << "[Logger] Info: Debug logging enabled – separate debug file will be written." << std::endl;
        }
    } else {
        auto console_backend = boost::make_shared<sinks::text_ostream_backend>();

        boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
        console_backend->add_stream(stream);

        using console_sink_t = sinks::asynchronous_sink<sinks::text_ostream_backend>;
        auto sink = boost::make_shared<console_sink_t>(console_backend);

        sink->set_filter(log::trivial::severity >= main_log_level);
        sink->set_formatter(CustomLogFormatter(log_format));

        sinks_.push_back(sink);
        log::core::get()->add_sink(sink);
    }
}

std::string Logger::GetProcessIdentifier() {
    static std::string process_identifier = []() {
        boost::uuids::random_generator gen;
        boost::uuids::uuid process_id = gen();
        return boost::uuids::to_string(process_id);
    }();
    return process_identifier;
}

#define DEFINE_LOG_METHOD(MethodName, BoostLevel)                                            \
    void Logger::MethodName(std::string_view message, LogSrcInfo src) {                      \
        BOOST_LOG_TRIVIAL(BoostLevel) << boost::log::add_value("File", src.file)             \
                                      << boost::log::add_value("Function", src.function)     \
                                      << boost::log::add_value("Line", src.line) << message; \
    }

DEFINE_LOG_METHOD(LogInfo, info)
DEFINE_LOG_METHOD(LogError, error)
DEFINE_LOG_METHOD(LogDebug, debug)
DEFINE_LOG_METHOD(LogWarn, warning)

#undef DEFINE_LOG_METHOD

}  // namespace utils
