#include "config.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ranges>
#include <unordered_map>

/**
 * Tricky parts:
 * - Flattening arbitrary JSON into a flat map of dot‑separated keys.
 * - Environment variable mapping: "server.port" → "SERVER_PORT".
 * - Using `boost::lexical_cast` for robust conversion (handles floats, bools, etc.).
 */

// Helper macro for debug dumping
#define PRINT_VAR(x) std::cout << std::left << std::setw(35) << #x << x << "\n"

namespace config {

// ----------------------------------------------------------------------------
// Internal helpers
// ----------------------------------------------------------------------------

namespace {

inline constexpr std::string_view kWarn = "[Settings] Warning:";
inline constexpr std::string_view kInfo = "[Settings] Info:";
inline constexpr std::string_view kFailConvert = "Failed to convert value for key";
inline constexpr std::string_view kUseDefault = "Using default.";
inline constexpr std::string_view kNewLine = "\n";

/// Cache for environment variables to avoid repeated `getenv` calls.
std::unordered_map<std::string, std::string> s_envCache;

/// Ensures the environment cache is initialised once (currently a no‑op, but reserved).
std::once_flag s_envInitFlag;

/// Ensures load called once
std::mutex s_loadMutex;
std::atomic<bool> s_loadDone{false};

using SettingHandler = std::function<void(Settings&, const std::string&)>;

// Basic types template (int, double, std::string)
template <typename FieldType>
void assign_field(FieldType& field, const std::string& value) {
    if constexpr (std::is_same_v<FieldType, std::string>) {
        field = value;
    } else {
        field = boost::lexical_cast<FieldType>(value);
    }
}

// Specialization for types from std::chrono::duration
template <typename Rep, typename Period>
void assign_field(std::chrono::duration<Rep, Period>& field, const std::string& value) {
    field = std::chrono::duration<Rep, Period>(boost::lexical_cast<Rep>(value));
}

// Helper for binding deeply nested fields (&Settings::server, &Settings::Server::host, etc.)
template <typename SubStruct, typename FieldType>
SettingHandler bind_field(SubStruct Settings::* sub_ptr, FieldType SubStruct::* field_ptr) {
    return [sub_ptr, field_ptr](Settings& s, const std::string& v) {
        assign_field((s.*sub_ptr).*field_ptr, v);
    };
}

const std::unordered_map<std::string, SettingHandler> kHandlers = {
    // Server
    {"server.host", bind_field(&Settings::server, &Settings::Server::host)},
    {"server.port", bind_field(&Settings::server, &Settings::Server::port)},
    {"server.max_players", bind_field(&Settings::server, &Settings::Server::max_players)},
    {"server.tick_rate_ms", bind_field(&Settings::server, &Settings::Server::tick_rate)},
    {"server.client_disconnect_timeout_sec",
     bind_field(&Settings::server, &Settings::Server::client_disconnect_timeout)},

    // Database
    {"database.connection_string", bind_field(&Settings::database, &Settings::Database::connection_string)},
    {"database.pool_size", bind_field(&Settings::database, &Settings::Database::pool_size)},

    // Gameplay
    {"gameplay.lobby_max_players", bind_field(&Settings::gameplay, &Settings::Gameplay::lobby_max_players)},
    {"gameplay.map_name", bind_field(&Settings::gameplay, &Settings::Gameplay::map_name)},
    {"gameplay.match_duration_sec", bind_field(&Settings::gameplay, &Settings::Gameplay::match_duration)},
    {"gameplay.player_default_hp", bind_field(&Settings::gameplay, &Settings::Gameplay::player_default_hp)},
    {"gameplay.monster_default_hp", bind_field(&Settings::gameplay, &Settings::Gameplay::monster_default_hp)},
    {"gameplay.monsters_per_player", bind_field(&Settings::gameplay, &Settings::Gameplay::monsters_per_player)},
    {"gameplay.default_radius_attack", bind_field(&Settings::gameplay, &Settings::Gameplay::default_radius_attack)},
    {"gameplay.default_radius_view", bind_field(&Settings::gameplay, &Settings::Gameplay::default_radius_view)},
    {"gameplay.default_map_blc_x", bind_field(&Settings::gameplay, &Settings::Gameplay::default_map_blc_x)},
    {"gameplay.default_map_blc_y", bind_field(&Settings::gameplay, &Settings::Gameplay::default_map_blc_y)},
    {"gameplay.default_map_trc_x", bind_field(&Settings::gameplay, &Settings::Gameplay::default_map_trc_x)},
    {"gameplay.default_map_trc_y", bind_field(&Settings::gameplay, &Settings::Gameplay::default_map_trc_y)},

    // Logger
    {"logger.level", bind_field(&Settings::logger, &Settings::Logger::level)},
    {"logger.output_dir", bind_field(&Settings::logger, &Settings::Logger::output_dir)},
    {"logger.format", bind_field(&Settings::logger, &Settings::Logger::format)},
};

}  // namespace

const Settings& Settings::instance() {
    return mutableInstance();
}

void Settings::initialize(const std::string& json_path) {
    if (!initialized_) {
        instance_ = std::unique_ptr<Settings>(new Settings());
        instance_->load(json_path);
        initialized_ = true;
    }
}

bool Settings::isInitialized() {
    return initialized_ && instance().loaded_;
}

std::optional<std::string> Settings::getEnv(const std::string& key) {
    // Ensure the cache is initialised (currently a placeholder for future use).
    std::call_once(s_envInitFlag, [] {
    });

    // Return from cache if present.
    auto it = s_envCache.find(key);
    if (it != s_envCache.end())
        return it->second;

    // Convert dots to underscores and uppercase: "server.port" → "SERVER_PORT"
    std::string envKey = key;
    std::ranges::replace(envKey, '.', '_');
    std::ranges::transform(envKey, envKey.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    if (const char* val = std::getenv(envKey.c_str())) {
        s_envCache[key] = val;  // Cache for future lookups.
        return std::string(val);
    }
    return std::nullopt;
}

void Settings::overrideSetting(const std::string& key, std::string value) {
    // Known keys – update the dedicated struct fields.
    // For numeric fields, conversion failures are caught and a warning printed.
    // The default value remains unchanged.
    auto it = kHandlers.find(key);

    if (it == kHandlers.end()) {
        dynamic_[key] = std::move(value);
        return;
    }

    try {
        it->second(*this, value);
    } catch (const boost::bad_lexical_cast&) {
        std::cerr << std::format("{} {} '{}'. {}", kWarn, kFailConvert, key, kUseDefault) << kNewLine;
    }
}

Settings& Settings::mutableInstance() {
    if (!initialized_ || !instance_) {
        throw std::logic_error("Settings not initialized – call initialize() first");
    }
    return *instance_;
}

void Settings::load(const std::string& json_path) {
    // Track first call
    if (s_loadDone.load(std::memory_order_acquire)) {
        std::cerr << std::format("{} load() called more than once – ignoring.", kWarn) << kNewLine;
        return;
    }

    // Lock mutex - block concurrent access
    std::lock_guard<std::mutex> lock(s_loadMutex);
    if (!s_loadDone.load(std::memory_order_relaxed)) {
        // 1. Start with hardcoded defaults (already set by member initializers).

        // 2. JSON file parsing and flattening.
        if (!json_path.empty()) {
            std::ifstream file(json_path, std::ios::binary);
            if (file.is_open()) {
                std::string input((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                boost::system::error_code ec;
                boost::json::value jv = boost::json::parse(input, ec);
                if (ec) {
                    throw std::runtime_error(std::format("JSON parse error: ", ec.message()));
                }
                std::cout << std::format("{} Loaded JSON configuration from '{}'", kInfo, json_path) << kNewLine;

                dynamic_.clear();         // очищаем перед заполнением
                flattenAndStore("", jv);  // теперь применяет известные ключи сразу
            } else {
                std::cerr << std::format("{} {} not found. {}", kWarn, json_path, kUseDefault) << kNewLine;
            }
        }

        // 3. Environment overrides, using kHandlers keys, highest priority
        std::vector<std::string> allKeys;
        allKeys.reserve(kHandlers.size() + dynamic_.size());

        // Все известные ключи
        for (const auto& key : kHandlers | std::views::keys) {
            allKeys.push_back(key);
        }
        // Плюс неизвестные ключи, которые могли появиться из JSON или быть добавлены вручную
        for (const auto& key : dynamic_ | std::views::keys) {
            allKeys.push_back(key);
        }

        for (const auto& key : allKeys) {
            if (auto envVal = getEnv(key)) {
                std::cout << std::format("{} ENV override: {} = {}", kInfo, key, *envVal) << kNewLine;
                overrideSetting(key, *envVal);
            }
        }

        // all done
        loaded_ = true;
        s_loadDone.store(true, std::memory_order_release);
    }
}

/**
 * @brief Convert a JSON object/array into flat map entries.
 * @param prefix Current dot‑separated path (e.g., "server").
 * @param jv Current JSON value.
 *
 * @details
 * - Objects: recurse into each member, appending ".key".
 * - Arrays: recurse into each element with index as key (e.g., "arr.0").
 * - Leaf values: stored as strings (true→"true", numbers→serialized).
 */
void Settings::flattenAndStore(std::string_view prefix, const boost::json::value& jv) {
    if (jv.is_object()) {
        for (const auto& item : jv.get_object()) {
            // Create new key: prefix + "." + item.key()
            std::string key;
            if (prefix.empty()) {
                key = std::string(item.key());
            } else {
                key.reserve(prefix.size() + 1 + item.key().size());
                key.append(prefix);
                key.push_back('.');
                key.append(item.key());
            }
            flattenAndStore(key, item.value());
        }
    } else if (jv.is_array()) {
        const auto& arr = jv.get_array();
        for (std::size_t i = 0; i < arr.size(); ++i) {
            std::string key;
            key.reserve(prefix.size() + 1 + std::to_string(i).size());
            key.append(prefix);
            key.push_back('.');
            key.append(std::to_string(i));
            flattenAndStore(key, arr[i]);
        }
    } else {
        // Leaf node – to string
        std::string value;
        if (jv.is_string()) {
            value = std::string(jv.get_string());
        } else if (jv.is_bool()) {
            value = jv.get_bool() ? "true" : "false";
        } else {
            // other types
            value = boost::json::serialize(jv);
        }

        std::string key(prefix);
        auto it = kHandlers.find(key);
        if (it != kHandlers.end()) {
            // Known key
            try {
                it->second(*this, value);
            } catch (const boost::bad_lexical_cast&) {
                std::cerr << std::format("{} {} '{}' from JSON. {}", kWarn, kFailConvert, key, kUseDefault) << kNewLine;
            }
        } else {
            //  Unknown key – store in dynamic_
            std::cout << std::format("{} Dynamic JSON key: {} = {} ", kInfo, key, value) << kNewLine;
            dynamic_[std::move(key)] = std::move(value);
        }
    }
}

std::string Settings::get(const std::string& key, std::string_view default_value) const {
    if (auto it = dynamic_.find(key); it != dynamic_.end())
        return it->second;
    return std::string(default_value);
}

void Settings::dump() const {
    std::cout << "\n============================================================\n";
    std::cout << "               FINAL CONFIGURATION (active)\n";
    std::cout << "============================================================\n";
    std::cout << std::left << std::setw(35) << "Key" << "Value\n";
    std::cout << "------------------------------------------------------------\n";

    // Known sections
    PRINT_VAR(server.host);
    PRINT_VAR(server.port);
    PRINT_VAR(server.max_players);
    PRINT_VAR(server.tick_rate.count());
    PRINT_VAR(database.connection_string);
    PRINT_VAR(database.pool_size);
    PRINT_VAR(gameplay.lobby_max_players);
    PRINT_VAR(gameplay.map_name);
    PRINT_VAR(gameplay.match_duration.count());
    PRINT_VAR(gameplay.player_default_hp);
    PRINT_VAR(gameplay.monster_default_hp);
    PRINT_VAR(gameplay.monsters_per_player);
    PRINT_VAR(gameplay.default_radius_attack);
    PRINT_VAR(gameplay.default_radius_view);
    PRINT_VAR(gameplay.default_map_blc_x);
    PRINT_VAR(gameplay.default_map_blc_y);
    PRINT_VAR(gameplay.default_map_trc_x);
    PRINT_VAR(gameplay.default_map_trc_y);
    PRINT_VAR(logger.level);
    PRINT_VAR(logger.output_dir);
    PRINT_VAR(logger.format);

    for (const auto& [key, val] : dynamic_) {
        std::cout << std::setw(35) << key << val << "\n";
    }
    std::cout << "============================================================\n";
}

#ifdef BUILD_TESTS
void Settings::clearEnvCacheForTesting() {
    s_envCache.clear();
}

void Settings::resetForTesting() {
    if (instance_) {
        instance_->loaded_ = false;
        instance_->dynamic_.clear();
        instance_->server = Server{};
        instance_->database = Database{};
        instance_->gameplay = Gameplay{};
        instance_->logger = Logger{};
        instance_.reset();  // delete the object
    }
    initialized_ = false;

    // Reset other global state (environment cache, load flag)
    s_loadDone.store(false, std::memory_order_release);
    s_envCache.clear();
}
#endif
}  // namespace config
