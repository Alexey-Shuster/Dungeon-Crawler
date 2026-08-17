#include <gtest/gtest.h>
#include <server/domain/dungeon/collision_check.h>
#include <server/domain/dungeon/entity_manager.h>
#include <server/domain/map/game_map.h>

using namespace dungeons::server::domain;

class CollisionCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a 10x10 map with some barriers
        map_ = std::make_unique<GameMap>(MapSize{Position{0, 0}, Position{9, 9}});
        map_->addBarrier(Position{4, 4});
        map_->addBarrier(Position{5, 5});

        // Add players
        players_.addEntity(PlayerId{1}, Position{2, 3});
        players_.addEntity(PlayerId{2}, Position{6, 7});

        // Add monsters
        monsters_.addEntity(MobId{100}, Position{8, 8});
        monsters_.addEntity(MobId{101}, Position{0, 0});

        checker_ = std::make_unique<CollisionChecker>(*map_, players_, monsters_);
    }

    std::unique_ptr<GameMap> map_;
    EntityManager<PlayerEntity, PlayerId> players_;
    EntityManager<MonsterEntity, MobId> monsters_;
    std::unique_ptr<CollisionChecker> checker_;
};

TEST_F(CollisionCheckerTest, AvailableWhenFree) {
    // Position not occupied, not barrier, inside map
    Position free{3, 3};
    EXPECT_TRUE(checker_->isAvailable(free));

    // Another free position
    Position free2{9, 0};
    EXPECT_TRUE(checker_->isAvailable(free2));
}

TEST_F(CollisionCheckerTest, BlockedByMapBarrier) {
    Position barrier{4, 4};
    EXPECT_FALSE(checker_->isAvailable(barrier));
}

TEST_F(CollisionCheckerTest, BlockedByPlayer) {
    Position playerPos{2, 3};
    EXPECT_FALSE(checker_->isAvailable(playerPos));
}

TEST_F(CollisionCheckerTest, BlockedByMonster) {
    Position monsterPos{8, 8};
    EXPECT_FALSE(checker_->isAvailable(monsterPos));
}

TEST_F(CollisionCheckerTest, BlockedByOutOfMap) {
    Position outOfMap{-1, 5};
    EXPECT_FALSE(checker_->isAvailable(outOfMap));
}

TEST_F(CollisionCheckerTest, MixedConditions) {
    // A position inside map, not barrier, but occupied by player -> blocked
    EXPECT_FALSE(checker_->isAvailable(Position{6, 7}));  // player

    // A position inside map, not barrier, occupied by monster -> blocked
    EXPECT_FALSE(checker_->isAvailable(Position{0, 0}));  // monster

    // A position inside map, barrier, no entity -> blocked
    EXPECT_FALSE(checker_->isAvailable(Position{5, 5}));  // barrier
}
