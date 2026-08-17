#pragma once

#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace dungeons::common::utility {
/**
 * @brief Singleton configuration manager.
 *
 * Thread-safe singleton (Meyers) that loads settings from:
 * 1. Hardcoded defaults
 * 2. A JSON file (if provided)
 * 3. Environment variables (highest priority, dot → underscore, uppercase)
 *
 * @note Modifications are only possible during `initialize()`. After that,
 *       all access is read-only.
 * @warning Do not call `initialize()` from multiple threads without external
 *          synchronization. Call it early in `main()` before spawning threads.
 *
 * Usage example:
 * @code
 * // Once at program start (e.g., in main())
 * config::Settings::initialize("config.json");
 *
 * // Anywhere else – read-only access
 * const auto& cfg = config::get_settings();
 * std::cout << cfg.server.port;
 * int custom = cfg.get<int>("mod.my_value", 0);
 * // WARN: do not use as global constant - app throws
 * @endcode
 */
class Settings {
public:
    // ========================================================================
    // Public nested structures – known configuration sections with defaults
    // ========================================================================

    struct Server {
        std::string host = "127.0.0.1";
        uint16_t port = 8080;
        uint16_t max_players = 100;
        std::chrono::milliseconds tick_rate = std::chrono::milliseconds(100);
        std::chrono::seconds client_disconnect_timeout = std::chrono::seconds(10);
    } server;

    struct Database {
        std::string connection_string{};
        uint16_t pool_size = 10;
    } database;

    struct Gameplay {
        uint16_t lobby_max_players = 4;
        std::string map_name = "test";
        std::chrono::seconds match_duration = std::chrono::seconds(100);
        uint32_t player_default_hp = 100;
        uint32_t monster_default_hp = 50;
        uint16_t monsters_per_player = 3;
        double default_radius_attack = 2;
        double default_radius_view = 4;
        uint32_t player_default_attack = 10;
        uint32_t monster_default_attack = 5;
        int64_t default_map_blc_x = 0;
        int64_t default_map_blc_y = 0;
        int64_t default_map_trc_x = 60;
        int64_t default_map_trc_y = 30;
    } gameplay;

    struct Logger {
        std::string level = "info";  // "debug", "info", "warn", "error"
        std::string output_dir = "./logs";
        std::string format = "json";  // "json" or "text"
    } logger;

    // ========================================================================
    // Dynamic (unknown) key access
    // ========================================================================

    /**
     * @brief Retrieve a dynamic configuration value by key.
     * @tparam T Target type (must be supported by boost::lexical_cast).
     * @param key Dot‑separated path, e.g., "mod.feature.timeout".
     * @param default_value Value returned if key does not exist or conversion fails.
     * @return Converted value or default.
     */
    template <typename T>
    T get(const std::string& key, const T& default_value = {}) const;

    /**
     * @brief Overload for string values (avoids lexical_cast).
     */
    [[nodiscard]] std::string get(const std::string& key, std::string_view default_value) const;

    /**
     * @brief Print all currently active settings to stdout (for debug).
     */
    void dump() const;

    // ========================================================================
    // Singleton access and initialization
    // ========================================================================

    /**
     * @brief Thread‑safe access to the singleton instance.
     * @return const reference to the unique Settings object.
     */
    static const Settings& instance();

    /**
     * @brief Load configuration from a JSON file and apply environment overrides.
     * @param json_path Path to JSON file. If empty, no file is read.
     * @note Must be called exactly once, preferably at the beginning of main().
     *       Subsequent calls are ignored and produce a warning.
     * @throw std::runtime_error If JSON parsing fails.
     */
    static void initialize(const std::string& json_path);

    Settings(const Settings&) = delete;

    Settings& operator=(const Settings&) = delete;

    Settings(Settings&&) = delete;

    Settings& operator=(Settings&&) = delete;

private:
    inline static std::unique_ptr<Settings> instance_ = nullptr;
    inline static bool initialized_ = false;

    Settings() = default;

    static Settings& mutableInstance();

    /**
     * @brief Internal load implementation (called by initialize()).
     */
    void load(const std::string& json_path);

    static bool isInitialized();

    /**
     * @brief Convert a string to type T using boost::lexical_cast.
     * @tparam T bool, arithmetic, or any type supported by lexical_cast.
     * @throws boost::bad_lexical_cast on failure.
     */
    template <typename T>
    static T convert(const std::string& s) {
        return boost::lexical_cast<T>(s);
    }

    /**
     * @brief Get an environment variable, converting dots to underscores.
     * @param key Dot‑separated key (e.g., "server.port").
     * @return Value if variable exists, std::nullopt otherwise.
     */
    static std::optional<std::string> getEnv(const std::string& key);

    /**
     * @brief Recursively flatten a JSON value into a flat map.
     * @param prefix Current dot‑separated path prefix (empty for root).
     * @param jv JSON value to flatten.
     *
     * @details Example: {"server":{"port":8080}} becomes one entry:
     *          key="server.port", value="8080".
     */
    void flattenAndStore(std::string_view prefix, const boost::json::value& jv);

    /**
     * @brief Override a single setting (known or dynamic) with a string value.
     * @param key Dot‑separated key.
     * @param value New value as string.
     *
     * @details Known keys update the corresponding struct fields.
     *          Unknown keys are stored in dynamic_ map.
     *          Numeric conversions use convert<T>() and on failure keep defaults.
     */
    void overrideSetting(const std::string& key, std::string value);

    boost::unordered_flat_map<std::string, std::string> dynamic_;
    bool loaded_ = false;

    inline static const std::unordered_set<std::string> kTrueValues = {"1", "true", "TRUE", "yes"};
    inline static const std::unordered_set<std::string> kFalseValues = {"0", "false", "FALSE", "no"};

    // Test hooks (only active when BUILD_TESTS is defined)
#ifdef BUILD_TESTS
public:
    static void resetForTesting();          ///< Reset singleton to pristine state.
    static void clearEnvCacheForTesting();  ///< Clear cached environment variables.
#endif
};

template <typename T>
T Settings::get(const std::string& key, const T& default_value) const {
    if (auto it = dynamic_.find(key); it != dynamic_.end()) {
        try {
            if constexpr (std::is_same_v<T, bool>) {
                const std::string& s = it->second;
                if (kTrueValues.contains(s))
                    return true;
                if (kFalseValues.contains(s))
                    return false;
                return boost::lexical_cast<bool>(s);  // fallback
            } else {
                return boost::lexical_cast<T>(it->second);
            }
        } catch (const boost::bad_lexical_cast&) {
            return default_value;
        }
    }
    return default_value;
}

/**
 * @brief Convenience global accessor.
 * @return const reference to the singleton Settings object.
 */
inline const Settings& getSettings() {
    return Settings::instance();
}
}  // namespace dungeons::common::utility
