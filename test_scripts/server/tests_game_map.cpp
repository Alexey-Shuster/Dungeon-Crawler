#include <gtest/gtest.h>
#include <server/domain/map/game_map.h>

using namespace dungeons::server::domain;

class GameMapTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a 10x10 map from (0,0) to (9,9)
        Position bl{0, 0};
        Position tr{9, 9};
        map_ = std::make_unique<GameMap>(MapSize{bl, tr});
    }

    std::unique_ptr<GameMap> map_;
};

TEST_F(GameMapTest, ConstructionAndSize) {
    EXPECT_EQ(map_->size().getBottomLeftCorner(), Position(0, 0));
    EXPECT_EQ(map_->size().getTopRightCorner(), Position(9, 9));
    EXPECT_TRUE(map_->getBarriers().empty());
}

TEST_F(GameMapTest, IsInMap) {
    EXPECT_TRUE(map_->isInMap(Position(0, 0)));
    EXPECT_TRUE(map_->isInMap(Position(9, 9)));
    EXPECT_TRUE(map_->isInMap(Position(5, 5)));
    EXPECT_FALSE(map_->isInMap(Position(-1, 0)));
    EXPECT_FALSE(map_->isInMap(Position(0, -1)));
    EXPECT_FALSE(map_->isInMap(Position(10, 5)));
    EXPECT_FALSE(map_->isInMap(Position(5, 10)));
}

TEST_F(GameMapTest, AddBarrier) {
    Position p{3, 4};
    EXPECT_TRUE(map_->addBarrier(p));
    EXPECT_TRUE(map_->isBarrier(p));
    EXPECT_EQ(map_->getBarriers().size(), 1);
    EXPECT_TRUE(map_->getBarriers().contains(p));

    // Adding duplicate barrier fails
    EXPECT_FALSE(map_->addBarrier(p));

    // Adding barrier outside map fails
    EXPECT_FALSE(map_->addBarrier(Position(-1, 0)));
}

TEST_F(GameMapTest, RemoveBarrier) {
    Position p{3, 4};
    map_->addBarrier(p);
    EXPECT_TRUE(map_->removeBarrier(p));
    EXPECT_FALSE(map_->isBarrier(p));
    EXPECT_TRUE(map_->getBarriers().empty());

    // Removing non-existent barrier fails
    EXPECT_FALSE(map_->removeBarrier(p));
}

TEST_F(GameMapTest, IsAvailable) {
    Position free{5, 5};
    EXPECT_TRUE(map_->isAvailable(free));  // inside map, no barrier

    Position barrierPos{3, 4};
    map_->addBarrier(barrierPos);
    EXPECT_FALSE(map_->isAvailable(barrierPos));

    Position outOfMap{10, 10};
    EXPECT_FALSE(map_->isAvailable(outOfMap));
}

TEST_F(GameMapTest, AddDuplicateBarrier) {
    Position p{3, 4};
    EXPECT_TRUE(map_->addBarrier(p));
    EXPECT_FALSE(map_->addBarrier(p));  // should fail
}
