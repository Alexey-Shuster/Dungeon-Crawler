#pragma once

#include <optional>
#include <unordered_map>

#include "../common/types.h"
#include "../map/position.h"

namespace dungeon {

template <typename EntityType, typename IdType>
class EntityManager {
public:
    using EntityMap = std::unordered_map<IdType, EntityType, StrongIdHash<IdType>>;

    const EntityMap& getEntities() const {
        return entities_;
    }

    // Non‑const access for modifications (caller must hold a lock)
    EntityMap& getEntities() {
        return entities_;
    }

    // Add an entity at a given position (assumes position is already validated)
    bool addEntity(IdType id, const map::Position& pos) {
        if (entities_.contains(id)) {
            return false;
        }
        entities_.emplace(id, EntityType{id, pos});
        return true;
    }

    // Move an entity if the new position is available (checked externally)
    bool moveEntity(IdType id, Direction dir, const map::Position& newPos) {
        auto it = entities_.find(id);
        if (it == entities_.end() || !it->second.isAlive()) {
            return false;
        }
        it->second.SetPosition(newPos);
        return true;
    }

    EntityType* getEntity(IdType id) {
        auto it = entities_.find(id);
        return (it != entities_.end()) ? &it->second : nullptr;
    }

    const EntityType* getEntity(IdType id) const {
        auto it = entities_.find(id);
        return (it != entities_.end()) ? &it->second : nullptr;
    }

    bool isOccupied(const map::Position& pos) const {
        for (const auto& [id, entity] : entities_) {
            if (entity.GetPosition() == pos) {
                return true;
            }
        }
        return false;
    }

    // Find the closest entity in 'targets' that is within attack radius from 'fromPos'
    template <typename TargetManager>
    auto findClosestTarget(const map::Position& fromPos, uint64_t radius, const TargetManager& targets) const {
        using TargetId = TargetManager::EntityMap::key_type;

        std::optional<TargetId> closest;
        uint64_t minDist = radius + 1;  // ensure we only consider within radius

        for (const auto& [targetId, target] : targets.getEntities()) {
            if (!target.isAlive())
                continue;
            uint64_t dist = fromPos.manhattanDistance(target.GetPosition());
            if (dist <= radius && dist < minDist) {
                minDist = dist;
                closest = targetId;

                if (dist == 0)
                    break;
            }
        }
        return closest;
    }

    void removeDeadEntities() {
        for (auto it = entities_.begin(); it != entities_.end();) {
            if (!it->second.isAlive()) {
                it = entities_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    EntityMap entities_;
};

}  // namespace dungeon
