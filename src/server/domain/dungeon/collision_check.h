#pragma once

#include "entity_manager.h"
#include "server/domain/core/types.h"
#include "server/domain/entity/entity.h"
#include "server/domain/map/game_map.h"

namespace dungeons::server::domain {

class CollisionChecker {

public:
    CollisionChecker(const GameMap& map,
                     const EntityManager<PlayerEntity, PlayerId>& players,
                     const EntityManager<MonsterEntity, MobId>& monsters)
        : map_(map)
        , players_(players)
        , monsters_(monsters) {}

    [[nodiscard]] bool isAvailable(const Position& pos) const {
        return map_.isAvailable(pos) && !players_.isOccupied(pos) && !monsters_.isOccupied(pos);
    }

private:
    const GameMap& map_;
    const EntityManager<PlayerEntity, PlayerId>& players_;
    const EntityManager<MonsterEntity, MobId>& monsters_;
};

}  // namespace dungeons::server::domain
