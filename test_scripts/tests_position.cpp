#include <gtest/gtest.h>
#include <limits>

#include "../src/domain/map/position.h"

using map::Position;

// ============================================================================
// Position tests
// ============================================================================

TEST(PositionTest, ConstructorAndAccessors) {
    Position p(3, 5);
    EXPECT_EQ(p.x, 3);
    EXPECT_EQ(p.y, 5);
}

TEST(PositionTest, EqualityComparison) {
    Position p1(1, 2);
    Position p2(1, 2);
    Position p3(2, 1);
    EXPECT_EQ(p1, p2);
    EXPECT_NE(p1, p3);
    EXPECT_NE(p2, p3);
}

TEST(PositionTest, OrderingComparison) {
    Position p1(1, 2);
    Position p2(1, 3);  // greater y
    Position p3(2, 2);  // greater x
    Position p4(1, 1);  // smaller y

    // Lexicographic order: x then y
    EXPECT_LT(p1, p2);  // x equal, y smaller
    EXPECT_LT(p1, p3);  // x smaller
    EXPECT_GT(p3, p1);
    EXPECT_GT(p2, p1);
    EXPECT_LT(p4, p1);
    EXPECT_GT(p2, p4);
}

TEST(PositionTest, BinaryAddition) {
    Position a(10, 20);
    Position b(5, -7);
    Position sum = a + b;
    EXPECT_EQ(sum.x, 15);
    EXPECT_EQ(sum.y, 13);

    // Addition with saturation
    Position max(std::numeric_limits<Position::Dimension>::max(), 100);
    Position delta(1, 0);
    Position saturated = max + delta;
    EXPECT_EQ(saturated.x, std::numeric_limits<Position::Dimension>::max());
    EXPECT_EQ(saturated.y, 100);  // unchanged

    Position min(std::numeric_limits<Position::Dimension>::min(), -100);
    Position delta_neg(-1, 0);
    Position saturated_min = min + delta_neg;
    EXPECT_EQ(saturated_min.x, std::numeric_limits<Position::Dimension>::min());
    EXPECT_EQ(saturated_min.y, -100);
}

TEST(PositionTest, BinarySubtraction) {
    Position a(10, 20);
    Position b(5, 7);
    Position diff = a - b;
    EXPECT_EQ(diff.x, 5);
    EXPECT_EQ(diff.y, 13);

    // Subtraction with saturation
    Position min(std::numeric_limits<Position::Dimension>::min(), 100);
    Position delta(1, 0);
    Position saturated = min - delta;
    EXPECT_EQ(saturated.x, std::numeric_limits<Position::Dimension>::min());
    EXPECT_EQ(saturated.y, 100);

    Position max(std::numeric_limits<Position::Dimension>::max(), -100);
    Position delta_neg(-1, 0);
    Position saturated_max = max - delta_neg;  // subtract negative -> add
    EXPECT_EQ(saturated_max.x, std::numeric_limits<Position::Dimension>::max());
    EXPECT_EQ(saturated_max.y, -100);
}

TEST(PositionTest, InPlaceAddition) {
    Position a(10, 20);
    Position b(5, -7);
    a += b;
    EXPECT_EQ(a.x, 15);
    EXPECT_EQ(a.y, 13);

    // Saturation
    Position max(std::numeric_limits<Position::Dimension>::max(), 100);
    Position delta(1, 0);
    max += delta;
    EXPECT_EQ(max.x, std::numeric_limits<Position::Dimension>::max());
    EXPECT_EQ(max.y, 100);
}

TEST(PositionTest, InPlaceSubtraction) {
    Position a(10, 20);
    Position b(5, 7);
    a -= b;
    EXPECT_EQ(a.x, 5);
    EXPECT_EQ(a.y, 13);

    // Saturation
    Position min(std::numeric_limits<Position::Dimension>::min(), 100);
    Position delta(1, 0);
    min -= delta;
    EXPECT_EQ(min.x, std::numeric_limits<Position::Dimension>::min());
    EXPECT_EQ(min.y, 100);
}

TEST(PositionTest, UnaryMinus) {
    Position p(10, -20);
    Position neg = -p;
    EXPECT_EQ(neg.x, -10);
    EXPECT_EQ(neg.y, 20);

    // Zero
    Position zero(0, 0);
    EXPECT_EQ(-zero, zero);

    // Minimum value: should wrap to maximum
    Position min_p(std::numeric_limits<Position::Dimension>::min(), std::numeric_limits<Position::Dimension>::min());
    Position neg_min = -min_p;
    EXPECT_EQ(neg_min.x, std::numeric_limits<Position::Dimension>::max());
    EXPECT_EQ(neg_min.y, std::numeric_limits<Position::Dimension>::max());

    // Maximum value
    Position max_p(std::numeric_limits<Position::Dimension>::max(), 0);
    Position neg_max = -max_p;
    EXPECT_EQ(neg_max.x, std::numeric_limits<Position::Dimension>::min() + 1);  // -max = min+1 (since min is -max-1)
    EXPECT_EQ(neg_max.y, 0);
}

TEST(PositionTest, ManhattanDistance) {
    Position p1(0, 0);
    Position p2(3, 4);
    EXPECT_DOUBLE_EQ(p1.manhattanDistance(p2), 7.0);
    EXPECT_DOUBLE_EQ(p2.manhattanDistance(p1), 7.0);

    Position p3(-2, -3);
    Position p4(5, 1);
    EXPECT_DOUBLE_EQ(p3.manhattanDistance(p4), 11.0);  // |5 - (-2)| + |1 - (-3)| = 7 + 4 = 11
}

TEST(PositionTest, EuclideanDistance) {
    Position p1(0, 0);
    Position p2(3, 4);
    EXPECT_DOUBLE_EQ(p1.euclideanDistance(p2), 5.0);
    EXPECT_DOUBLE_EQ(p2.euclideanDistance(p1), 5.0);

    Position p3(1, 2);
    Position p4(4, 6);
    EXPECT_DOUBLE_EQ(p3.euclideanDistance(p4), 5.0);  // sqrt(3^2+4^2)
}

TEST(PositionTest, DistanceSymmetric) {
    Position a(10, 20);
    Position b(30, 15);
    EXPECT_DOUBLE_EQ(a.manhattanDistance(b), b.manhattanDistance(a));
    EXPECT_DOUBLE_EQ(a.euclideanDistance(b), b.euclideanDistance(a));
}
