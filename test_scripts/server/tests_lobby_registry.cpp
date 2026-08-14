#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <server/domain/lobby/lobby.h>
#include <server/domain/lobby/lobby_registry.h>

using namespace dungeons::server::domain;

class LobbyRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = std::make_unique<LobbyRegistry>();
    }

    std::unique_ptr<LobbyRegistry> registry;
};

TEST_F(LobbyRegistryTest, FindLobbyExists) {
    auto lobby = std::make_shared<Lobby>(LobbyId(13), PlayerId(1));
    registry->addLobby(lobby);

    auto found = registry->findLobby(LobbyId(13));
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), LobbyId(13));
}

TEST_F(LobbyRegistryTest, FindLobbyNotExists) {
    auto found = registry->findLobby(LobbyId(999));
    EXPECT_EQ(found, nullptr);
}
