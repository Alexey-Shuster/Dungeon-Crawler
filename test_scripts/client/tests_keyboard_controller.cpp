#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <ui/keyboard_controller.h>

using namespace dungeons::client::ui;

TEST(KeyboardControllerTest, MoveKeysToCommands) {
    EXPECT_EQ(commandFromKey('w'), "MOVE UP");
    EXPECT_EQ(commandFromKey('W'), "MOVE UP");

    EXPECT_EQ(commandFromKey('a'), "MOVE LEFT");
    EXPECT_EQ(commandFromKey('s'), "MOVE DOWN");
    EXPECT_EQ(commandFromKey('D'), "MOVE RIGHT");
    EXPECT_EQ(commandFromKey('d'), "MOVE RIGHT");
}

TEST(KeyboardControllerTest, AttackKeyToCommand) {
    EXPECT_EQ(commandFromKey('x'), "ATTACK");
    EXPECT_EQ(commandFromKey('X'), "ATTACK");
}

TEST(KeyboardControllerTest, IgnoreUnknownKeys) {
    EXPECT_EQ(commandFromKey('q'), std::nullopt);
    EXPECT_EQ(commandFromKey('e'), std::nullopt);
    EXPECT_EQ(commandFromKey('z'), std::nullopt);
    EXPECT_EQ(commandFromKey('c'), std::nullopt);
    EXPECT_EQ(commandFromKey(']'), std::nullopt);
    EXPECT_EQ(commandFromKey('/'), std::nullopt);
    EXPECT_EQ(commandFromKey('0'), std::nullopt);
    EXPECT_EQ(commandFromKey(' '), std::nullopt);
}
