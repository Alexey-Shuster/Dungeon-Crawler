#pragma once

#include "domain/entity/entity.h"
#include "domain/map/game_map.h"
#include "domain/types.h"

namespace dungeons::server::domain {

struct DungeonState {
    GameMap game_map;
    std::unordered_map<PlayerId, PlayerEntity, PlayerHash> players;
    std::unordered_map<MobId, MonsterEntity, MobHash> monsters;
};

}  // namespace dungeons::server::domain
