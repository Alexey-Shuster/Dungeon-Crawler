#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <common/game_state_dto.h>
#include <ui/render_game_state.h>

using namespace dungeons::client::ui;

// Helper to create a simple map with given bounds and optional barriers
static ::network::DungeonSnapshot makeMap(uint64_t blc_x,
                                          uint64_t blc_y,
                                          uint64_t trc_x,
                                          uint64_t trc_y,
                                          std::vector<::network::BarrierSnapshot> barriers = {},
                                          std::vector<::network::EntitySnapshot> entities = {}) {
    ::network::DungeonSnapshot snap;
    snap.game_map.blc_x = blc_x;
    snap.game_map.blc_y = blc_y;
    snap.game_map.trc_x = trc_x;
    snap.game_map.trc_y = trc_y;
    snap.game_map.barriers = std::move(barriers);
    snap.entities = std::move(entities);
    return snap;
}

TEST(RenderGameStateTest, EmptyMap) {
    auto snap = makeMap(0, 0, 2, 2);
    auto lines = renderGameState(snap);

    std::vector<std::string> expected =
        {"=== Game State ===", "#####", "#...#", "#...#", "#...#", "#####", "Entities: 0"};
    EXPECT_EQ(lines, expected);
}

TEST(RenderGameStateTest, WithBarriers) {
    auto snap = makeMap(0, 0, 2, 2, {{1, 1}, {0, 2}});
    auto lines = renderGameState(snap);

    std::vector<std::string> expected = {"=== Game State ===",
                                         "#####",
                                         "#b..#",  // y=2: barrier at (0,2)
                                         "#.b.#",  // y=1: barrier at (1,1)
                                         "#...#",  // y=0: none
                                         "#####",
                                         "Entities: 0"};
    EXPECT_EQ(lines, expected);
}

TEST(RenderGameStateTest, WithEntities) {
    auto snap =
        makeMap(0, 0, 2, 2, {}, {{kEntityTypePlayer, 0, 100, 1, 1, 10}, {kEntityTypeMonster, 0, 200, 2, 0, 20}});
    auto lines = renderGameState(snap);

    std::vector<std::string> expected = {"=== Game State ===",
                                         "#####",
                                         "#...#",  // y=2
                                         "#.@.#",  // y=1: player at (1,1)
                                         "#..M#",  // y=0: monster at (2,0)
                                         "#####",
                                         "Entities: 2",
                                         "  Entity 100 (Player) HP: 10",
                                         "  Entity 200 (Monster) HP: 20"};
    EXPECT_EQ(lines, expected);
}

TEST(RenderGameStateTest, EntitiesOverwriteBarriers) {
    auto snap = makeMap(0, 0, 2, 2, {{1, 1}}, {{kEntityTypePlayer, 0, 100, 1, 1, 10}});
    auto lines = renderGameState(snap);

    std::vector<std::string> expected = {"=== Game State ===",
                                         "#####",
                                         "#...#",
                                         "#.@.#",  // Player overwrites barrier
                                         "#...#",
                                         "#####",
                                         "Entities: 1",
                                         "  Entity 100 (Player) HP: 10"};
    EXPECT_EQ(lines, expected);
}

TEST(RenderGameStateTest, NonZeroOrigin) {
    auto snap = makeMap(5, 5, 7, 7, {{6, 6}}, {{kEntityTypePlayer, 0, 100, 5, 7, 10}});
    auto lines = renderGameState(snap);

    std::vector<std::string> expected = {"=== Game State ===",
                                         "#####",
                                         "#@..#",  // y=7: player at (5,7)
                                         "#.b.#",  // y=6: barrier at (6,6)
                                         "#...#",  // y=5: none
                                         "#####",
                                         "Entities: 1",
                                         "  Entity 100 (Player) HP: 10"};
    EXPECT_EQ(lines, expected);
}

TEST(RenderGameStateTest, LargeMapExceedsLimit) {
    auto snap = makeMap(0, 0, 200, 200);
    auto lines = renderGameState(snap);

    ASSERT_EQ(lines.size(), 1);
    EXPECT_TRUE(lines[0].find("Game map too large to display") != std::string::npos);
}

TEST(RenderGameStateTest, UnknownEntityType) {
    auto snap = makeMap(0, 0, 2, 2, {}, {{99, 0, 100, 1, 1, 10}});  // type 99
    auto lines = renderGameState(snap);

    std::vector<std::string> expected = {"=== Game State ===",
                                         "#####",
                                         "#...#",
                                         "#.?.#",
                                         "#...#",
                                         "#####",
                                         "Entities: 1",
                                         "  Entity 100 (Unknown) HP: 10"};
    EXPECT_EQ(lines, expected);
}

TEST(RenderGameStateTest, MultipleEntitiesSameCell) {
    auto snap =
        makeMap(0, 0, 2, 2, {}, {{kEntityTypePlayer, 0, 100, 1, 1, 10}, {kEntityTypeMonster, 0, 200, 1, 1, 20}});
    auto lines = renderGameState(snap);

    std::vector<std::string> expected = {"=== Game State ===",
                                         "#####",
                                         "#...#",
                                         "#.M.#",  // Monster (last) overwrites player
                                         "#...#",
                                         "#####",
                                         "Entities: 2",
                                         "  Entity 100 (Player) HP: 10",
                                         "  Entity 200 (Monster) HP: 20"};
    EXPECT_EQ(lines, expected);
}
