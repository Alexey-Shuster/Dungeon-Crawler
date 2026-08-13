#include <gtest/gtest.h>

#include "../src/server/lobby/lobby.h"
#include "config.h"

using namespace lobby;

class LobbyTest : public ::testing::Test {
protected:
    void SetUp() override {
        lobby = std::make_unique<Lobby>(LobbyId{42}, PlayerId{1});
        lobby->addPlayer(PlayerId{1});
    }

    std::unique_ptr<Lobby> lobby;
};

// ---------------------------------------------------------------------------
// Constructor and basic getters
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, Constructor_InitialState) {
    EXPECT_EQ(lobby->getId(), LobbyId{42});
    EXPECT_EQ(lobby->getPlayerCount(), 1);  // leader is already present
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});
}

// ---------------------------------------------------------------------------
// addPlayer / containsPlayer / getPlayerCount / leader
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, AddPlayer_Success) {
    PlayerId pid{2};
    EXPECT_TRUE(lobby->addPlayer(pid));
    EXPECT_TRUE(lobby->containsPlayer(pid));
    EXPECT_EQ(lobby->getPlayerCount(), 2);
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});  // leader remains unchanged
}

TEST_F(LobbyTest, AddPlayer_Duplicate_ReturnsFalse) {
    PlayerId pid{2};
    EXPECT_TRUE(lobby->addPlayer(pid));
    EXPECT_FALSE(lobby->addPlayer(pid));
    EXPECT_EQ(lobby->getPlayerCount(), 2);
}

TEST_F(LobbyTest, AddPlayer_SecondPlayer_DoesNotChangeLeader) {
    PlayerId p2{2};
    EXPECT_TRUE(lobby->addPlayer(p2));
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});
}

TEST_F(LobbyTest, ContainsPlayer_NonExistent_ReturnsFalse) {
    EXPECT_FALSE(lobby->containsPlayer(PlayerId{999}));
}

// ---------------------------------------------------------------------------
// removePlayer
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, RemovePlayer_Existing_ReturnsTrue) {
    PlayerId pid{2};
    lobby->addPlayer(pid);
    EXPECT_TRUE(lobby->removePlayer(pid));
    EXPECT_FALSE(lobby->containsPlayer(pid));
    EXPECT_EQ(lobby->getPlayerCount(), 1);
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});
}

TEST_F(LobbyTest, RemovePlayer_NonExistent_ReturnsFalse) {
    EXPECT_FALSE(lobby->removePlayer(PlayerId{999}));
}

TEST_F(LobbyTest, RemovePlayer_LeaderLeaves_AssignsNewLeader) {
    PlayerId p2{2};
    lobby->addPlayer(p2);
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});

    EXPECT_TRUE(lobby->removePlayer(PlayerId{1}));
    EXPECT_EQ(lobby->getPlayerCount(), 1);
    // The new leader is the first element in the unordered_map.
    // With the current implementation, that will be p2 if it was inserted after the leader.
    // (This is deterministic enough for this test, but keep in mind that unordered_map
    //  iteration order is not guaranteed by the standard.)
    EXPECT_EQ(lobby->getLeader(), p2);
}

TEST_F(LobbyTest, RemovePlayer_NonLeaderLeaves_LeaderUnchanged) {
    PlayerId p2{2};
    lobby->addPlayer(p2);
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});

    EXPECT_TRUE(lobby->removePlayer(p2));
    EXPECT_EQ(lobby->getPlayerCount(), 1);
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});
}

// The lobby is never empty while alive; the manager deletes it when the
// last player leaves, so we don't test the empty-state case here.

// ---------------------------------------------------------------------------
// setReady
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, SetReady_ExistingPlayer_Success) {
    PlayerId pid{2};
    lobby->addPlayer(pid);
    EXPECT_TRUE(lobby->setReady(pid, true));
    // We can indirectly verify readiness via checkAllReady().
    EXPECT_FALSE(lobby->checkAllReady());  // leader is still not ready
}

TEST_F(LobbyTest, SetReady_NonExistentPlayer_ReturnsFalse) {
    EXPECT_FALSE(lobby->setReady(PlayerId{999}, true));
}

// ---------------------------------------------------------------------------
// checkAllReady
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, CheckAllReady_AllReady_ReturnsTrue) {
    PlayerId p2{2};
    lobby->addPlayer(p2);
    lobby->setReady(PlayerId{1}, true);
    lobby->setReady(p2, true);
    EXPECT_TRUE(lobby->checkAllReady());
}

TEST_F(LobbyTest, CheckAllReady_NotAllReady_ReturnsFalse) {
    PlayerId p2{2};
    lobby->addPlayer(p2);
    lobby->setReady(PlayerId{1}, true);  // p2 not ready
    EXPECT_FALSE(lobby->checkAllReady());
}

TEST_F(LobbyTest, CheckAllReady_SinglePlayerReady_ReturnsTrue) {
    // Only the leader exists; set leader ready
    lobby->setReady(PlayerId{1}, true);
    EXPECT_TRUE(lobby->checkAllReady());
}

TEST_F(LobbyTest, CheckAllReady_SinglePlayerNotReady_ReturnsFalse) {
    EXPECT_FALSE(lobby->checkAllReady());  // leader default is false
}

// ---------------------------------------------------------------------------
// Combined scenarios (no state machine)
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, FullLifecycle_AddRemoveAndReadiness) {
    PlayerId p2{2}, p3{3};

    EXPECT_TRUE(lobby->addPlayer(p2));
    EXPECT_TRUE(lobby->addPlayer(p3));
    EXPECT_EQ(lobby->getLeader(), PlayerId{1});

    // Set all ready
    EXPECT_TRUE(lobby->setReady(PlayerId{1}, true));
    EXPECT_TRUE(lobby->setReady(p2, true));
    EXPECT_TRUE(lobby->setReady(p3, true));
    EXPECT_TRUE(lobby->checkAllReady());

    // Remove a player – readiness is not re‑evaluated automatically,
    // but the player is gone, so checkAllReady() will now consider only
    // the remaining players.
    EXPECT_TRUE(lobby->removePlayer(p3));
    EXPECT_EQ(lobby->getPlayerCount(), 2);
    EXPECT_TRUE(lobby->checkAllReady());  // remaining two are still ready

    // Remove the leader – new leader assigned (p2)
    EXPECT_TRUE(lobby->removePlayer(PlayerId{1}));
    EXPECT_EQ(lobby->getPlayerCount(), 1);
    EXPECT_EQ(lobby->getLeader(), p2);
    // p2 is still ready, so all ready still holds
    EXPECT_TRUE(lobby->checkAllReady());

    // Remove last player – lobby would be deleted by manager, but we can
    // still test removal and count.
    EXPECT_TRUE(lobby->removePlayer(p2));
    EXPECT_EQ(lobby->getPlayerCount(), 0);
    // leaderId_ is now stale (p2), but the manager should delete the lobby
    // before accessing it again.
}

// ---------------------------------------------------------------------------
// Getters (additional)
// ---------------------------------------------------------------------------
TEST_F(LobbyTest, GetId_ReturnsCorrectId) {
    LobbyId id{123};
    Lobby other(id, PlayerId{5});
    EXPECT_EQ(other.getId(), id);
}
