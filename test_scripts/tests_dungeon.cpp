#include <chrono>
#include <gtest/gtest.h>
#include <memory>

#include "../src/common/config.h"
#include "../src/common/logger.h"
#include "../src/domain/dungeon/dungeon.h"
#include "../src/domain/map/game_map.h"

using namespace dungeon;
using namespace map;

// Helper to get the current dungeon state after processing an empty tick.
// This is used to inspect entity positions and health.
static std::optional<DungeonState> tickAndGetState(Dungeon& d,
                                                   std::chrono::milliseconds dt = std::chrono::milliseconds{1}) {
    return d.processTick(dt);
}

// Helper to compute Manhattan distance.
static uint64_t distance(const Position& a, const Position& b) {
    return a.manhattanDistance(b);
}

TEST(DungeonTest, ConstructorCreatesPlayersAndMonsters) {
    GameMap game_map(MapSize{Position{0, 0}, Position{20, 20}});
    std::vector<PlayerId> players = {PlayerId{1}, PlayerId{2}, PlayerId{3}};
    Dungeon dungeon(std::move(game_map), players);

    // After construction, we need to process a tick to get the state
    auto state = tickAndGetState(dungeon);
    ASSERT_TRUE(state.has_value());

    EXPECT_EQ(state->players.size(), players.size());
    size_t expected_monsters = players.size() * config::getSettings().gameplay.monsters_per_player;
    EXPECT_EQ(state->monsters.size(), expected_monsters);
}

TEST(DungeonTest, CollisionPreventsOverlap) {
    GameMap game_map(MapSize{Position{0, 0}, Position{20, 20}});
    PlayerId p1{1}, p2{2};
    Dungeon dungeon(std::move(game_map), std::vector<PlayerId>{p1, p2});

    // Get initial state and find positions
    auto state = tickAndGetState(dungeon);
    ASSERT_TRUE(state.has_value());
    auto pos1 = state->players.at(p1).GetPosition();
    auto pos2 = state->players.at(p2).GetPosition();

    // Move p1 towards p2
    Direction dir;
    if (pos2.x > pos1.x)
        dir = Direction::kRight;
    else if (pos2.x < pos1.x)
        dir = Direction::kLeft;
    else if (pos2.y > pos1.y)
        dir = Direction::kUp;
    else
        dir = Direction::kDown;

    // Try to move into p2's position
    dungeon.addMovePlayerCommand(p1, dir);
    state = tickAndGetState(dungeon);
    ASSERT_TRUE(state.has_value());

    auto new_pos1 = state->players.at(p1).GetPosition();
    auto new_pos2 = state->players.at(p2).GetPosition();
    EXPECT_NE(new_pos1, new_pos2);
}

TEST(DungeonTest, MonstersMoveAndAttackOverTicks) {
    // This test checks that after a few ticks, monsters have moved or attacked.
    GameMap game_map(MapSize{Position{0, 0}, Position{20, 20}});
    PlayerId p1{1};
    Dungeon dungeon(std::move(game_map), std::vector<PlayerId>{p1});

    // Get initial state
    auto state = tickAndGetState(dungeon);
    ASSERT_TRUE(state.has_value());
    auto initial_monster_positions = state->monsters;
    auto initial_player_health = state->players.at(p1).getHealth();

    // Process several ticks to allow AI to act
    for (int i = 0; i < 5; ++i) {
        state = tickAndGetState(dungeon);
        ASSERT_TRUE(state.has_value());
    }

    // Check that either some monster moved or player health decreased.
    bool monster_moved = false;
    for (const auto& [id, monster] : state->monsters) {
        auto init_it = initial_monster_positions.find(id);
        if (init_it != initial_monster_positions.end()) {
            if (init_it->second.GetPosition() != monster.GetPosition()) {
                monster_moved = true;
                break;
            }
        }
    }
    bool player_damaged = (state->players.at(p1).getHealth() < initial_player_health);

    // It's possible neither happened (randomness), but we can expect at least one.
    EXPECT_TRUE(monster_moved || player_damaged);
}
