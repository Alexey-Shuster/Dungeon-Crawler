#pragma once

#include "core/types.h"
#include "entity/entity.h"
#include "map/game_map.h"

namespace dungeons::server::domain {

struct DungeonState {
    GameMap game_map;
    std::unordered_map<PlayerId, PlayerEntity, PlayerHash> players;
    std::unordered_map<MobId, MonsterEntity, MobHash> monsters;
};

}  // namespace dungeons::server::domain
