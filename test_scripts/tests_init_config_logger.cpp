#include <gtest/gtest.h>

#include "../common/config.h"
#include "../common/logger.h"

class GlobalSetupEnvironment : public ::testing::Environment {
public:
    ~GlobalSetupEnvironment() override = default;

    void SetUp() override {
        config::Settings::initialize("");
        utils::Logger::Initialize();
        utils::Logger::Instance();
    }

    void TearDown() override {
        utils::Logger::Reset();
    }
};

[[maybe_unused]] const ::testing::Environment* const g_env =
    ::testing::AddGlobalTestEnvironment(new GlobalSetupEnvironment);
