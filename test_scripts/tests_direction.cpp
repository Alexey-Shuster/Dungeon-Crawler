#include <gtest/gtest.h>

#include "../src/domain/map/direction.h"
#include "../src/domain/map/position.h"

using map::Position;

TEST(DirectionTest, PositionOffsetFromDirection) {
    Position up = positionOffsetFromDirection(map::Direction::kUp);
    EXPECT_EQ(up.x, 0);
    EXPECT_EQ(up.y, 1);

    Position down = map::positionOffsetFromDirection(map::Direction::kDown);
    EXPECT_EQ(down.x, 0);
    EXPECT_EQ(down.y, -1);

    Position left = map::positionOffsetFromDirection(map::Direction::kLeft);
    EXPECT_EQ(left.x, -1);
    EXPECT_EQ(left.y, 0);

    Position right = map::positionOffsetFromDirection(map::Direction::kRight);
    EXPECT_EQ(right.x, 1);
    EXPECT_EQ(right.y, 0);
}

TEST(DirectionTest, OffsetValues) {
    // Verify offsets match expected directions
    auto offset = map::positionOffsetFromDirection(map::Direction::kUp);
    EXPECT_EQ(offset, Position(0, 1));
    offset = map::positionOffsetFromDirection(map::Direction::kDown);
    EXPECT_EQ(offset, Position(0, -1));
    offset = map::positionOffsetFromDirection(map::Direction::kLeft);
    EXPECT_EQ(offset, Position(-1, 0));
    offset = map::positionOffsetFromDirection(map::Direction::kRight);
    EXPECT_EQ(offset, Position(1, 0));
}
