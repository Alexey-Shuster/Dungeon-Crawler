#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <common/direction.h>
#include <server/domain/map/position.h>

using namespace dungeons::server::domain;
using namespace types;

TEST(DirectionTest, PositionOffsetFromDirection) {
    Position up = positionOffsetFromDirection(Direction::kUp);
    EXPECT_EQ(up.x, 0);
    EXPECT_EQ(up.y, 1);

    Position down = positionOffsetFromDirection(Direction::kDown);
    EXPECT_EQ(down.x, 0);
    EXPECT_EQ(down.y, -1);

    Position left = positionOffsetFromDirection(Direction::kLeft);
    EXPECT_EQ(left.x, -1);
    EXPECT_EQ(left.y, 0);

    Position right = positionOffsetFromDirection(Direction::kRight);
    EXPECT_EQ(right.x, 1);
    EXPECT_EQ(right.y, 0);
}

TEST(DirectionTest, OffsetValues) {
    // Verify offsets match expected directions
    auto offset = positionOffsetFromDirection(Direction::kUp);
    EXPECT_EQ(offset, Position(0, 1));
    offset = positionOffsetFromDirection(Direction::kDown);
    EXPECT_EQ(offset, Position(0, -1));
    offset = positionOffsetFromDirection(Direction::kLeft);
    EXPECT_EQ(offset, Position(-1, 0));
    offset = positionOffsetFromDirection(Direction::kRight);
    EXPECT_EQ(offset, Position(1, 0));
}
