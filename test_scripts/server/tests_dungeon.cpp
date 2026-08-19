#include <common/network/game_state_dto.h>
#include <gtest/gtest.h>
#include <memory>
#include <server/domain/dungeon/dungeon.h>
#include <server/domain/map/game_map.h>
#include <vector>

using namespace dungeons::server::domain;
using namespace dungeons::common::types;
using namespace dungeons::common::network;

// -----------------------------------------------------------------------------
// Helper: create a map with known size and no barriers
// -----------------------------------------------------------------------------
static GameMap createTestMap(uint32_t width = 10, uint32_t height = 10) {
    MapSize size(Position{0, 0}, Position{width - 1, height - 1});
    return GameMap(size);
}

// -----------------------------------------------------------------------------
// Fixture: sets up a Dungeon with 2 players on a 10x10 map.
// The number of monsters depends on the config (monsters_per_player).
// -----------------------------------------------------------------------------
class DungeonTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto map = createTestMap();
        std::vector<PlayerId> playerIds = {PlayerId{1}, PlayerId{2}};
        dungeon_ = std::make_shared<Dungeon>(std::move(map), std::move(playerIds));
    }

    // Process one tick and return the snapshot (never null unless game over)
    std::shared_ptr<DungeonSnapshot> tick() {
        return dungeon_->processTick(std::chrono::milliseconds(0));
    }

    // Find an entity (player or mob) by ID in a snapshot
    const EntitySnapshot* findEntity(const DungeonSnapshot& snap, uint64_t id) const {
        for (const auto& e : snap.players)
            if (e.id == id)
                return &e;
        for (const auto& e : snap.mobs)
            if (e.id == id)
                return &e;
        return nullptr;
    }

    std::shared_ptr<Dungeon> dungeon_;
};

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

TEST_F(DungeonTest, ConstructionSpawnsPlayersAndMonsters) {
    auto snap = tick();
    ASSERT_NE(snap, nullptr);

    EXPECT_EQ(snap->players.size(), 2);
    // Monsters count: players * monsters_per_player (configurable)
    // We just check that there is at least one.
    EXPECT_GT(snap->mobs.size(), 0);
}

TEST_F(DungeonTest, MultipleCommandsProcessedInOneTick) {
    dungeon_->addMovePlayerCommand(PlayerId{1}, Direction::kRight);
    dungeon_->addMovePlayerCommand(PlayerId{2}, Direction::kLeft);
    dungeon_->addPlayerAttackCommand(PlayerId{1}, 10);

    auto snap = tick();
    ASSERT_NE(snap, nullptr);
    // Just ensure no crash; both moves and attack are processed.
    SUCCEED();
}

TEST_F(DungeonTest, AttackCommandDoesNotCrash) {
    // Attack might hit a monster if one is in range, but we don't check damage.
    // Just verify the command is processed without errors.
    dungeon_->addPlayerAttackCommand(PlayerId{1}, 5);
    auto snap = tick();
    ASSERT_NE(snap, nullptr);
    SUCCEED();
}

TEST_F(DungeonTest, GetPlayersReturnsCorrectIds) {
    auto ids = dungeon_->getPlayers();
    EXPECT_EQ(ids.size(), 2);
    bool found1 = false, found2 = false;
    for (auto id : ids) {
        if (id.get() == 1)
            found1 = true;
        if (id.get() == 2)
            found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST_F(DungeonTest, ProcessTickAlwaysReturnsSnapshotUntilGameOver) {
    // Just check that a few ticks produce snapshots.
    for (int i = 0; i < 5; ++i) {
        auto snap = tick();
        EXPECT_NE(snap, nullptr);
    }
}
