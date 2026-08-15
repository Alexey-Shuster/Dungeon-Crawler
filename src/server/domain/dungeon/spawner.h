#pragma once

#include <common/utility/logger.h>
#include <format>

#include "collision_check.h"
#include "entity_manager.h"
#include "position_gen.h"

namespace dungeons::server::domain {

constexpr size_t kMaxSpawnAttempts = 100;

class Spawner {
public:
    Spawner(PositionGenerator& pos_gen, const CollisionChecker& checker)
        : pos_gen_(pos_gen)
        , checker_(checker) {}

    template <typename EntityType, typename IdType>
    bool spawnEntity(EntityManager<EntityType, IdType>& manager, IdType id) {
        if (manager.getEntity(id) != nullptr) {
            LOG_DEBUG(std::format("Entity with id {} already exists", id.value));
            return false;
        }

        size_t attempts = 1;
        auto pos = pos_gen_.generate();

        while (!checker_.isAvailable(pos) && attempts <= kMaxSpawnAttempts) {
            ++attempts;
            LOG_DEBUG(std::format("Position ({},{}) not available for entity {}, attempt {}/{}",
                                  pos.x,
                                  pos.y,
                                  id.value,
                                  attempts,
                                  kMaxSpawnAttempts));
            pos = pos_gen_.generate();
        }

        if (!checker_.isAvailable(pos)) {
            LOG_ERROR(std::format("Failed to spawn entity {}: no free position after {} attempts",
                                  id.value,
                                  kMaxSpawnAttempts));
            return false;
        }

        bool ok = manager.addEntity(id, pos);

        if (ok) {
            LOG_DEBUG(std::format("Entity {} spawned at ({},{}) after {} attempts", id.value, pos.x, pos.y, attempts));
        }
        return ok;
    }

private:
    PositionGenerator& pos_gen_;
    const CollisionChecker& checker_;
};

}  // namespace dungeons::server::domain
