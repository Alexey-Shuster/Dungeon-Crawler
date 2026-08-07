#include <gtest/gtest.h>

#include "../src/client/keyboard_controller.h"

TEST(KeyboardControllerTest, MoveKeysToCommands) {
    EXPECT_EQ(network::keyboard_controller::commandFromKey('w'), "MOVE UP");
    EXPECT_EQ(network::keyboard_controller::commandFromKey('W'), "MOVE UP");

    EXPECT_EQ(network::keyboard_controller::commandFromKey('a'), "MOVE LEFT");
    EXPECT_EQ(network::keyboard_controller::commandFromKey('s'), "MOVE DOWN");
    EXPECT_EQ(network::keyboard_controller::commandFromKey('D'), "MOVE RIGHT");
    EXPECT_EQ(network::keyboard_controller::commandFromKey('d'), "MOVE RIGHT");
}

TEST(KeyboardControllerTest, AttackKeyToCommand) {
    EXPECT_EQ(network::keyboard_controller::commandFromKey('x'), "ATTACK");
    EXPECT_EQ(network::keyboard_controller::commandFromKey('X'), "ATTACK");
}

TEST(KeyboardControllerTest, IgnoreUnknownKeys) {
    EXPECT_EQ(network::keyboard_controller::commandFromKey('q'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey('e'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey('z'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey('c'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey(']'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey('/'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey('1'), std::nullopt);
    EXPECT_EQ(network::keyboard_controller::commandFromKey(' '), std::nullopt);
}
