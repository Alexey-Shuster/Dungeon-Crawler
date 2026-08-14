#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <common/config.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
inline void setenv(const char* name, const char* value, int overwrite) {
    if (overwrite || getenv(name) == nullptr) {
        std::string eq = std::string(name) + "=" + value;
        _putenv(eq.c_str());
    }
}

inline void unsetenv(const char* name) {
    std::string eq = std::string(name) + "=";
    _putenv(eq.c_str());
}
#else
    #include <unistd.h>  // for setenv/unsetenv
#endif

namespace fs = std::filesystem;

class SettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        config::Settings::resetForTesting();

        // Unset all environment variables that might interfere
        unsetenv("SERVER_HOST");
        unsetenv("SERVER_PORT");
        unsetenv("SERVER_MAX_PLAYERS");
        unsetenv("SERVER_TICK_RATE_MS");
        unsetenv("SERVER_CLIENT_DISCONNECT_TIMEOUT_SEC");
        unsetenv("DATABASE_CONNECTION_STRING");
        unsetenv("DATABASE_POOL_SIZE");
        unsetenv("GAMEPLAY_LOBBY_MAX_PLAYERS");
        unsetenv("GAMEPLAY_MAP_NAME");
        unsetenv("GAMEPLAY_MATCH_DURATION_SEC");
        unsetenv("GAMEPLAY_DEFAULT_MAP_TRC_X");
        unsetenv("GAMEPLAY_DEFAULT_MAP_TRC_Y");
        unsetenv("LOGGER_LEVEL");
        unsetenv("LOGGER_OUTPUT_DIR");
        unsetenv("MOD_CUSTOM_VALUE");
        unsetenv("MOD_FOO");
        unsetenv("MOD_BOOL_TEST");
        unsetenv("MOD_FLAG");
    }

    void TearDown() override {
        if (fs::exists("test_config.json"))
            fs::remove("test_config.json");

        unsetenv("SERVER_HOST");
        unsetenv("SERVER_PORT");
        unsetenv("SERVER_MAX_PLAYERS");
        unsetenv("SERVER_TICK_RATE_MS");
        unsetenv("SERVER_CLIENT_DISCONNECT_TIMEOUT_SEC");
        unsetenv("DATABASE_CONNECTION_STRING");
        unsetenv("DATABASE_POOL_SIZE");
        unsetenv("GAMEPLAY_LOBBY_MAX_PLAYERS");
        unsetenv("GAMEPLAY_MAP_NAME");
        unsetenv("GAMEPLAY_MATCH_DURATION_SEC");
        unsetenv("GAMEPLAY_DEFAULT_MAP_TRC_X");
        unsetenv("GAMEPLAY_DEFAULT_MAP_TRC_Y");
        unsetenv("LOGGER_LEVEL");
        unsetenv("LOGGER_OUTPUT_DIR");
        unsetenv("MOD_CUSTOM_VALUE");
        unsetenv("MOD_FOO");
        unsetenv("MOD_BOOL_TEST");
        unsetenv("MOD_FLAG");
    }

    void writeConfig(const std::string& content) {
        std::ofstream file("test_config.json");
        if (!file.is_open()) {
            std::cerr << "Cannot create test_config.json" << std::endl;
            return;
        }
        file << content;
    }
};

// ------------------------------------------------------------------
// Basic loading from JSON
// ------------------------------------------------------------------
TEST_F(SettingsTest, LoadFromJson) {
    writeConfig(R"({
        "server": {
            "host": "10.0.0.1",
            "port": 9090,
            "max_players": 200,
            "tick_rate_ms": 20,
            "client_disconnect_timeout_sec": 30
        },
        "database": {
            "connection_string": "postgres://test",
            "pool_size": 50
        },
        "gameplay": {
            "lobby_max_players": 8,
            "map_name": "deathmatch",
            "match_duration_sec": 600
        },
        "logger": {
            "level": "debug",
            "output_dir": "/tmp/logs"
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.host, "10.0.0.1");
    EXPECT_EQ(cfg.server.port, 9090);
    EXPECT_EQ(cfg.server.max_players, 200);
    EXPECT_EQ(cfg.server.tick_rate.count(), 20);
    EXPECT_EQ(cfg.server.client_disconnect_timeout.count(), 30);
    EXPECT_EQ(cfg.database.connection_string, "postgres://test");
    EXPECT_EQ(cfg.database.pool_size, 50);
    EXPECT_EQ(cfg.gameplay.lobby_max_players, 8);
    EXPECT_EQ(cfg.gameplay.map_name, "deathmatch");
    EXPECT_EQ(cfg.gameplay.match_duration.count(), 600);
    EXPECT_EQ(cfg.logger.level, "debug");
    EXPECT_EQ(cfg.logger.output_dir, "/tmp/logs");
}

// ------------------------------------------------------------------
// Default values for ALL known fields
// ------------------------------------------------------------------
TEST_F(SettingsTest, DefaultValuesForAllKnownFields) {
    // No file, no env – all defaults must be present
    config::Settings::initialize("");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.host, "127.0.0.1");
    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.server.max_players, 100);
    EXPECT_EQ(cfg.server.tick_rate.count(), 100);
    EXPECT_EQ(cfg.server.client_disconnect_timeout.count(), 60);
    EXPECT_EQ(cfg.database.connection_string, "");
    EXPECT_EQ(cfg.database.pool_size, 10);
    EXPECT_EQ(cfg.gameplay.lobby_max_players, 4);
    EXPECT_EQ(cfg.gameplay.map_name, "test");
    EXPECT_EQ(cfg.gameplay.match_duration.count(), 100);
    EXPECT_EQ(cfg.logger.level, "info");
    EXPECT_EQ(cfg.logger.output_dir, "./logs");
}

// ------------------------------------------------------------------
// Environment overrides for ALL known fields
// ------------------------------------------------------------------
TEST_F(SettingsTest, EnvironmentOverridesAllKnownFields) {
    setenv("SERVER_HOST", "env.host", 1);
    setenv("SERVER_PORT", "9999", 1);
    setenv("SERVER_MAX_PLAYERS", "250", 1);
    setenv("SERVER_TICK_RATE_MS", "50", 1);
    setenv("SERVER_CLIENT_DISCONNECT_TIMEOUT_SEC", "60", 1);
    setenv("DATABASE_CONNECTION_STRING", "env_db_string", 1);
    setenv("DATABASE_POOL_SIZE", "25", 1);
    setenv("GAMEPLAY_LOBBY_MAX_PLAYERS", "12", 1);
    setenv("GAMEPLAY_MAP_NAME", "env_map", 1);
    setenv("GAMEPLAY_MATCH_DURATION_SEC", "900", 1);
    setenv("LOGGER_LEVEL", "warn", 1);
    setenv("LOGGER_OUTPUT_DIR", "/env/logs", 1);

    config::Settings::initialize("");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.host, "env.host");
    EXPECT_EQ(cfg.server.port, 9999);
    EXPECT_EQ(cfg.server.max_players, 250);
    EXPECT_EQ(cfg.server.tick_rate.count(), 50);
    EXPECT_EQ(cfg.server.client_disconnect_timeout.count(), 60);
    EXPECT_EQ(cfg.database.connection_string, "env_db_string");
    EXPECT_EQ(cfg.database.pool_size, 25);
    EXPECT_EQ(cfg.gameplay.lobby_max_players, 12);
    EXPECT_EQ(cfg.gameplay.map_name, "env_map");
    EXPECT_EQ(cfg.gameplay.match_duration.count(), 900);
    EXPECT_EQ(cfg.logger.level, "warn");
    EXPECT_EQ(cfg.logger.output_dir, "/env/logs");
}

// ------------------------------------------------------------------
// Environment overrides for unknown dynamic keys
// ------------------------------------------------------------------
TEST_F(SettingsTest, EnvironmentOverrideDynamicKey) {
    // Create JSON containing the dynamic key so it appears in dynamic_
    writeConfig(R"({
        "mod": {
            "foo": "dummy"
        }
    })");

    setenv("MOD_FOO", "bar", 1);
    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.get<std::string>("mod.foo", ""), "bar");
}

// ------------------------------------------------------------------
// Boolean conversion with various truthy/falsy strings
// ------------------------------------------------------------------
TEST_F(SettingsTest, BooleanConversionVariants) {
    writeConfig(R"({
        "mod": {
            "bool_true_1": 1,
            "bool_true_yes": "yes",
            "bool_true_TRUE": "TRUE",
            "bool_false_0": 0,
            "bool_false_no": "no",
            "bool_false_FALSE": "FALSE",
            "bool_invalid": "maybe"
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_TRUE(cfg.get<bool>("mod.bool_true_1", false));
    EXPECT_TRUE(cfg.get<bool>("mod.bool_true_yes", false));
    EXPECT_TRUE(cfg.get<bool>("mod.bool_true_TRUE", false));
    EXPECT_FALSE(cfg.get<bool>("mod.bool_false_0", true));
    EXPECT_FALSE(cfg.get<bool>("mod.bool_false_no", true));
    EXPECT_FALSE(cfg.get<bool>("mod.bool_false_FALSE", true));
    // Invalid string → fallback to default (false)
    EXPECT_FALSE(cfg.get<bool>("mod.bool_invalid", false));
}

// ------------------------------------------------------------------
// Array flattening
// ------------------------------------------------------------------
TEST_F(SettingsTest, ArrayFlattening) {
    writeConfig(R"({
        "arr": [10, 20, 30],
        "nested": {
            "list": ["a", "b"]
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.get<int>("arr.0", 0), 10);
    EXPECT_EQ(cfg.get<int>("arr.1", 0), 20);
    EXPECT_EQ(cfg.get<int>("arr.2", 0), 30);
    EXPECT_EQ(cfg.get<std::string>("nested.list.0", ""), "a");
    EXPECT_EQ(cfg.get<std::string>("nested.list.1", ""), "b");
}

// ------------------------------------------------------------------
// Empty string values for numeric fields
// ------------------------------------------------------------------
TEST_F(SettingsTest, EmptyStringNumericFallsBackToDefault) {
    writeConfig(R"({
        "server": {
            "port": "",
            "max_players": ""
        },
        "database": {
            "pool_size": ""
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    // Empty string conversion fails → keep defaults
    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.server.max_players, 100);
    EXPECT_EQ(cfg.database.pool_size, 10);
}

// ------------------------------------------------------------------
// Environment cache clearing
// ------------------------------------------------------------------
TEST_F(SettingsTest, EnvironmentCacheClearing) {
    // Write JSON with the dynamic key
    writeConfig(R"({
        "mod": {
            "foo": "dummy"
        }
    })");

    setenv("MOD_FOO", "first", 1);
    config::Settings::initialize("test_config.json");
    const auto& cfg1 = config::getSettings();
    EXPECT_EQ(cfg1.get<std::string>("mod.foo", ""), "first");

    // Reset and change env
    config::Settings::resetForTesting();
    config::Settings::clearEnvCacheForTesting();  // ensure cache cleared
    setenv("MOD_FOO", "second", 1);
    // Re‑write the same JSON (or keep existing file; it still exists)
    writeConfig(R"({
        "mod": {
            "foo": "dummy"
        }
    })");
    config::Settings::initialize("test_config.json");
    const auto& cfg2 = config::getSettings();
    EXPECT_EQ(cfg2.get<std::string>("mod.foo", ""), "second");
}

// ------------------------------------------------------------------
// Dynamic keys (basic)
// ------------------------------------------------------------------
TEST_F(SettingsTest, DynamicKeys) {
    writeConfig(R"({
        "mod": {
            "myFloat": 3.14,
            "myBool": true,
            "myString": "hello"
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_DOUBLE_EQ(cfg.get<double>("mod.myFloat", 0.0), 3.14);
    EXPECT_TRUE(cfg.get<bool>("mod.myBool", false));
    EXPECT_EQ(cfg.get<std::string>("mod.myString", ""), "hello");
    EXPECT_EQ(cfg.get<int>("mod.nonExistent", 999), 999);
}

// ------------------------------------------------------------------
// Conversion errors fall back to default
// ------------------------------------------------------------------
TEST_F(SettingsTest, ConversionErrorUsesDefault) {
    writeConfig(R"({
        "server": { "max_players": "not_an_int" },
        "database": { "pool_size": "xyz" }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.max_players, 100);
    EXPECT_EQ(cfg.database.pool_size, 10);
}

// ------------------------------------------------------------------
// Global reference matches instance
// ------------------------------------------------------------------
TEST_F(SettingsTest, GlobalReferenceMatchesInstance) {
    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();
    const auto& instance = config::Settings::instance();
    EXPECT_EQ(&cfg, &instance);
}

// ------------------------------------------------------------------
// Empty path skips file
// ------------------------------------------------------------------
TEST_F(SettingsTest, EmptyPathSkipsFile) {
    setenv("SERVER_MAX_PLAYERS", "250", 1);
    config::Settings::initialize("");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.max_players, 250);
    EXPECT_EQ(cfg.server.host, "127.0.0.1");
}

// ------------------------------------------------------------------
// StringView default
// ------------------------------------------------------------------
TEST_F(SettingsTest, StringViewDefault) {
    writeConfig(R"({ "mod": { "test": "trace" } })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    std::string value = cfg.get("mod.test", std::string_view("info"));
    EXPECT_EQ(value, "trace");

    std::string missing = cfg.get("nonexistent.key", std::string_view("defaultVal"));
    EXPECT_EQ(missing, "defaultVal");
}

// ------------------------------------------------------------------
// Reset allows reinitialization
// ------------------------------------------------------------------
TEST_F(SettingsTest, ResetAllowsReinitialization) {
    writeConfig(R"({ "server": { "port": 1111 } })");
    config::Settings::initialize("test_config.json");
    EXPECT_EQ(config::getSettings().server.port, 1111);

    config::Settings::resetForTesting();
    writeConfig(R"({ "server": { "port": 2222 } })");
    config::Settings::initialize("test_config.json");
    EXPECT_EQ(config::getSettings().server.port, 2222);
}

// ------------------------------------------------------------------
// Dump output (visual inspection)
// ------------------------------------------------------------------
TEST_F(SettingsTest, DumpOutput) {
    writeConfig(R"({
        "server": {
            "host": "192.168.1.100",
            "port": 8888,
            "max_players": 150,
            "tick_rate_ms": 50
        },
        "database": {
            "pool_size": 25
        },
        "gameplay": {
            "map_name": "ctf",
            "match_duration_sec": 450
        },
        "mod": {
            "custom_float": 12.34,
            "custom_flag": true
        }
    })");

    setenv("SERVER_PORT", "9999", 1);
    setenv("LOGGER_LEVEL", "warn", 1);
    setenv("MOD_CUSTOM_STRING", "hello_env", 1);

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();
    cfg.dump();
    EXPECT_TRUE(true);
}

// ------------------------------------------------------------------
// Known key override is applied to struct, but get() returns default
// ------------------------------------------------------------------
TEST_F(SettingsTest, KnownKeyOverrideDirectAccessVsGet) {
    writeConfig(R"({
        "gameplay": {
            "default_map_trc_x": 40,
            "default_map_trc_y": 20
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    // Direct struct access sees the override
    EXPECT_EQ(cfg.gameplay.default_map_trc_x, 40);
    EXPECT_EQ(cfg.gameplay.default_map_trc_y, 20);

    // get() does NOT see it – it looks only in dynamic_
    EXPECT_EQ(cfg.get<int>("gameplay.default_map_trc_x", 100), 100);
    EXPECT_EQ(cfg.get<int>("gameplay.default_map_trc_y", 25), 25);
}

// ------------------------------------------------------------------
// Environment override for known key: direct access works, get() does not
// ------------------------------------------------------------------
TEST_F(SettingsTest, EnvOverrideKnownKeyDirectAccessVsGet) {
    setenv("GAMEPLAY_DEFAULT_MAP_TRC_X", "50", 1);
    setenv("GAMEPLAY_DEFAULT_MAP_TRC_Y", "30", 1);

    config::Settings::initialize("");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.gameplay.default_map_trc_x, 50);
    EXPECT_EQ(cfg.gameplay.default_map_trc_y, 30);

    EXPECT_EQ(cfg.get<int>("gameplay.default_map_trc_x", 100), 100);
    EXPECT_EQ(cfg.get<int>("gameplay.default_map_trc_y", 25), 25);
}

// ------------------------------------------------------------------
// Environment overrides JSON (highest priority)
// ------------------------------------------------------------------
TEST_F(SettingsTest, EnvOverridesJson) {
    writeConfig(R"({
        "server": {
            "port": 1111,
            "host": "json.host"
        }
    })");
    setenv("SERVER_PORT", "2222", 1);
    setenv("SERVER_HOST", "env.host", 1);

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.port, 2222);
    EXPECT_EQ(cfg.server.host, "env.host");
}

// ------------------------------------------------------------------
// JSON keys are case‑sensitive – only exact match works
// ------------------------------------------------------------------
TEST_F(SettingsTest, CaseSensitiveKeys) {
    writeConfig(R"({
        "server": {
            "host": "lowercase",
            "Host": "uppercase"
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    // "host" is known, "Host" is not – will end up in dynamic_
    EXPECT_EQ(cfg.server.host, "lowercase");
    EXPECT_EQ(cfg.get<std::string>("server.Host", ""), "uppercase");
}

// ------------------------------------------------------------------
// Dynamic key: JSON and env override, get() works
// ------------------------------------------------------------------
TEST_F(SettingsTest, DynamicKeyWithEnvOverride) {
    writeConfig(R"({
        "mod": {
            "foo": "json_value"
        }
    })");
    setenv("MOD_FOO", "env_value", 1);

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    // get() sees the environment value because dynamic_ is updated
    EXPECT_EQ(cfg.get<std::string>("mod.foo", ""), "env_value");
}

// ------------------------------------------------------------------
// Invalid JSON throws runtime_error
// ------------------------------------------------------------------
TEST_F(SettingsTest, InvalidJsonThrows) {
    writeConfig(R"({
        "server": { "port": 8080,  // missing closing brace
    })");

    EXPECT_THROW(config::Settings::initialize("test_config.json"), std::runtime_error);
}

// ------------------------------------------------------------------
// Missing JSON file – defaults used, warning printed (no exception)
// ------------------------------------------------------------------
TEST_F(SettingsTest, MissingJsonFileUsesDefaults) {
    // No file created
    config::Settings::initialize("non_existent.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.port, 8080);
    EXPECT_EQ(cfg.gameplay.default_map_trc_x, 60);
    // No exception
}

// ------------------------------------------------------------------
// get() on dynamic key with invalid number returns default
// ------------------------------------------------------------------
TEST_F(SettingsTest, GetDynamicInvalidNumberReturnsDefault) {
    writeConfig(R"({
        "mod": {
            "bad_int": "not_a_number"
        }
    })");

    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.get<int>("mod.bad_int", 999), 999);
}

// ------------------------------------------------------------------
// Environment variable keys are uppercase with underscores (non‑Windows)
// ------------------------------------------------------------------
#ifndef _WIN32
TEST_F(SettingsTest, EnvKeyUppercaseWithUnderscores) {
    setenv("SERVER_PORT", "1234", 1);  // correct
    setenv("server_port", "5678", 1);  // ignored – wrong case

    config::Settings::initialize("");
    const auto& cfg = config::getSettings();

    EXPECT_EQ(cfg.server.port, 1234);
}
#endif

// ------------------------------------------------------------------
// Second initialize() call is ignored
// ------------------------------------------------------------------
TEST_F(SettingsTest, DoubleInitializationIgnored) {
    writeConfig(R"({ "server": { "port": 9999 } })");
    config::Settings::initialize("test_config.json");
    const auto& cfg1 = config::getSettings();
    EXPECT_EQ(cfg1.server.port, 9999);

    writeConfig(R"({ "server": { "port": 8888 } })");
    config::Settings::initialize("test_config.json");
    const auto& cfg2 = config::getSettings();
    // Same object, so port remains 9999
    EXPECT_EQ(cfg2.server.port, 9999);
    // Ensure they are the same instance
    EXPECT_EQ(&cfg1, &cfg2);
}

// ------------------------------------------------------------------
// Environment boolean
// ------------------------------------------------------------------
TEST_F(SettingsTest, EnvBooleanFalse) {
    // Add a boolean known key? There's no known bool in structs.
    // Use dynamic instead.
    writeConfig(R"({ "mod": { "flag": true } })");
    setenv("MOD_FLAG", "false", 1);
    config::Settings::initialize("test_config.json");
    const auto& cfg = config::getSettings();
    EXPECT_FALSE(cfg.get<bool>("mod.flag", true));
}
