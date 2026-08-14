#include <gtest/gtest.h>
#include <common/utility/config.h>
#include <common/utility/logger.h>

class GlobalSetupEnvironment : public ::testing::Environment {
public:
    ~GlobalSetupEnvironment() override = default;

    void SetUp() override {
        using namespace dungeons::common::utility;
        Settings::initialize("");
        Logger::Initialize();
        Logger::Instance();
    }

    void TearDown() override {
        dungeons::common::utility::Logger::Reset();
    }
};

[[maybe_unused]] const ::testing::Environment* const g_env =
    ::testing::AddGlobalTestEnvironment(new GlobalSetupEnvironment);
