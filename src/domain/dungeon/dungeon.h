#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

#include "../../common/direction.h"
#include "../../common/types.h"
#include "../entity/entity.h"
#include "../map/game_map.h"
#include "collision_check.h"
#include "command_queue.h"
#include "direction_gen.h"
#include "entity_manager.h"
#include "monster_control.h"
#include "position_gen.h"
#include "spawner.h"

namespace dungeon {

struct DungeonState {
    map::GameMap game_map;
    std::unordered_map<PlayerId, entity::PlayerEntity, StrongIdHash<PlayerId>> players;
    std::unordered_map<MobId, entity::MonsterEntity, StrongIdHash<MobId>> monsters;
};

class Dungeon : public std::enable_shared_from_this<Dungeon> {
public:
    template <typename PlayersContainer>
    explicit Dungeon(map::GameMap game_map, PlayersContainer&& initial_players);

    void addPlayerAttackCommand(PlayerId player_id, uint32_t damage);
    void addMovePlayerCommand(PlayerId player_id, Direction direction);
    std::optional<DungeonState> processTick(std::chrono::milliseconds time_delta);
    std::vector<PlayerId> getPlayers() const;

private:
    bool isGameOver() const;
    void incrementMobCounter() {
        ++mob_id_counter_;
    }
    MobId nextMobId() {
        return MobId{mob_id_counter_++};
    }

    map::GameMap game_map_;
    EntityManager<entity::PlayerEntity, PlayerId> players_;
    EntityManager<entity::MonsterEntity, MobId> monsters_;
    CollisionChecker collision_checker_;
    PositionGenerator pos_gen_;
    DirectionGenerator dir_gen_;
    Spawner spawner_;
    CommandQueue command_queue_;
    MonsterController monster_ai_;

    std::atomic<size_t> mob_id_counter_{0};

    // Mutex for protecting entity modifications (players_ and monsters_)
    mutable std::mutex entities_mutex_;
};

template <typename PlayersContainer>
Dungeon::Dungeon(map::GameMap game_map, PlayersContainer&& initial_players)
    : game_map_(std::move(game_map))
    , collision_checker_(game_map_, players_, monsters_)
    , pos_gen_(game_map_)
    , spawner_(pos_gen_, collision_checker_)
    , monster_ai_(monsters_, players_, collision_checker_, dir_gen_) {

    // Add players
    for (auto& playerId : initial_players) {
        spawner_.spawnEntity(players_, playerId);
    }

    // Add monsters
    size_t playerCount = initial_players.size();
    size_t totalMonsters = playerCount * config::getSettings().gameplay.monsters_per_player;
    for (size_t i = 0; i < totalMonsters; ++i) {
        spawner_.spawnEntity(monsters_, nextMobId());
    }
}

}  // namespace dungeon
