#pragma once

#include "../entity/entity.h"
#include "../map/game_map.h"
#include "entity_manager.h"

namespace dungeon {

class CollisionChecker {
public:
    CollisionChecker(const map::GameMap& map,
                     const EntityManager<entity::PlayerEntity, PlayerId>& players,
                     const EntityManager<entity::MonsterEntity, MobId>& monsters)
        : map_(map)
        , players_(players)
        , monsters_(monsters) {}

    bool isAvailable(const map::Position& pos) const {
        return map_.isAvailable(pos) && !players_.isOccupied(pos) && !monsters_.isOccupied(pos);
    }

private:
    const map::GameMap& map_;
    const EntityManager<entity::PlayerEntity, PlayerId>& players_;
    const EntityManager<entity::MonsterEntity, MobId>& monsters_;
};

}  // namespace dungeon
