#include "config.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
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
// Internal helpers (file‑static)
// ----------------------------------------------------------------------------

namespace {
/// Cache for environment variables to avoid repeated `getenv` calls.
std::unordered_map<std::string, std::string> s_envCache;

/// Ensures the environment cache is initialised once (currently a no‑op, but reserved).
std::once_flag s_envInitFlag;

/// Ensures load called once
std::mutex s_loadMutex;
std::atomic<bool> s_loadDone{false};

using SettingHandler = std::function<void(Settings&, const std::string&)>;

const std::unordered_map<std::string, SettingHandler> kHandlers = {
    {"server.host",
     [](Settings& s, const std::string& v) {
         s.server.host = v;
     }},
    {"server.port",
     [](Settings& s, const std::string& v) {
         s.server.port = boost::lexical_cast<int>(v);
     }},
    {"server.max_players",
     [](Settings& s, const std::string& v) {
         s.server.max_players = boost::lexical_cast<int>(v);
     }},
    {"server.tick_rate_ms",
     [](Settings& s, const std::string& v) {
         s.server.tick_rate = std::chrono::milliseconds(boost::lexical_cast<int>(v));
     }},
    {"server.client_disconnect_timeout_sec",
     [](Settings& s, const std::string& v) {
         s.server.client_disconnect_timeout = std::chrono::seconds(boost::lexical_cast<int>(v));
     }},
    {"database.connection_string",
     [](Settings& s, const std::string& v) {
         s.database.connection_string = v;
     }},
    {"database.pool_size",
     [](Settings& s, const std::string& v) {
         s.database.pool_size = boost::lexical_cast<int>(v);
     }},
    {"gameplay.lobby_max_players",
     [](Settings& s, const std::string& v) {
         s.gameplay.lobby_max_players = boost::lexical_cast<int>(v);
     }},
    {"gameplay.map_name",
     [](Settings& s, const std::string& v) {
         s.gameplay.map_name = v;
     }},
    {"gameplay.match_duration_sec",
     [](Settings& s, const std::string& v) {
         s.gameplay.match_duration = std::chrono::seconds(boost::lexical_cast<int>(v));
     }},
    {"gameplay.player_default_hp",
     [](Settings& s, const std::string& v) {
         s.gameplay.player_default_hp = boost::lexical_cast<int>(v);
     }},
    {"gameplay.monster_default_hp",
     [](Settings& s, const std::string& v) {
         s.gameplay.monster_default_hp = boost::lexical_cast<int>(v);
     }},
    {"gameplay.max_monsters_per_player",
     [](Settings& s, const std::string& v) {
         s.gameplay.monsters_per_player = boost::lexical_cast<int>(v);
     }},
    {"gameplay.default_radius_attack",
     [](Settings& s, const std::string& v) {
         s.gameplay.default_radius_attack = boost::lexical_cast<double>(v);
     }},
    {"gameplay.default_radius_view",
     [](Settings& s, const std::string& v) {
         s.gameplay.default_radius_view = boost::lexical_cast<double>(v);
     }},
    {"gameplay.default_map_blc_x",
     [](Settings& s, const std::string& v) {
         s.gameplay.default_radius_view = boost::lexical_cast<int>(v);
     }},
    {"gameplay.default_map_blc_y",
     [](Settings& s, const std::string& v) {
         s.gameplay.default_radius_view = boost::lexical_cast<int>(v);
     }},
    {"gameplay.default_map_trc_x",
     [](Settings& s, const std::string& v) {
         s.gameplay.default_radius_view = boost::lexical_cast<int>(v);
     }},
    {"gameplay.default_map_trc_y",
     [](Settings& s, const std::string& v) {
         s.gameplay.default_radius_view = boost::lexical_cast<int>(v);
     }},
    {"logger.level",
     [](Settings& s, const std::string& v) {
         s.logger.level = v;
     }},
    {"logger.output_dir",
     [](Settings& s, const std::string& v) {
         s.logger.output_dir = v;
     }},
};
}  // namespace

const Settings& Settings::instance() {
    return mutableInstance();
}

void Settings::initialize(const std::string& json_path) {
    mutableInstance().load(json_path);
}

bool Settings::isInitialized() {
    return instance().loaded_;
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
        std::cerr << "[Settings] Warning: Failed to convert value for key '" << key << "'. Using default.\n";
    }
}

Settings& Settings::mutableInstance() {
    static Settings instance;
    return instance;
}

void Settings::load(const std::string& json_path) {
    // Track first call
    if (s_loadDone.load(std::memory_order_acquire)) {
        std::cerr << "[Settings] Warning: load() called more than once – ignoring.\n";
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
                    throw std::runtime_error("JSON parse error: " + ec.message());
                }

                dynamic_.clear();         // очищаем перед заполнением
                flattenAndStore("", jv);  // теперь применяет известные ключи сразу
            } else {
                std::cerr << "[Settings] Warning: " << json_path << " not found, using defaults.\n";
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
                std::cerr << "[Settings] Warning: Failed to convert value for key '" << key
                          << "' from JSON. Using default.\n";
            }
        } else {
            //  Unknown key
            dynamic_[std::move(key)] = std::move(value);
        }
    }
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
    Settings& mutable_self = const_cast<Settings&>(instance());
    mutable_self.loaded_ = false;
    mutable_self.dynamic_.clear();
    mutable_self.server = Server{};
    mutable_self.database = Database{};
    mutable_self.gameplay = Gameplay{};
    mutable_self.logger = Logger{};

    // Reset flags for testing
    s_loadDone.store(false, std::memory_order_release);
    s_envCache.clear();
}
#endif
}  // namespace config
