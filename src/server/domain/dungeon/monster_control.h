#pragma once

#include <random>

#include "collision_check.h"
#include "direction_gen.h"
#include "domain/entity/entity.h"
#include "entity_manager.h"

namespace dungeons::server::domain {

class MonsterController {
public:
    MonsterController(EntityManager<MonsterEntity, MobId>& monsters,
                      EntityManager<PlayerEntity, PlayerId>& players,
                      const CollisionChecker& checker,
                      DirectionGenerator& dir_gen)
        : monsters_(monsters)
        , players_(players)
        , checker_(checker)
        , dir_gen_(dir_gen) {}

    // Move up to 'maxMoves' random monsters (but at most the number of monsters)
    void moveRandomMonsters(size_t maxMoves) {
        auto& monsterMap = monsters_.getEntities();
        if (monsterMap.empty())
            return;

        size_t moves = std::min(maxMoves, monsterMap.size());
        // random index distribution
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, monsterMap.size() - 1);

        for (size_t i = 0; i < moves; ++i) {
            size_t idx = dist(gen);
            auto it = std::next(monsterMap.begin(), idx);
            if (!it->second.isAlive())
                continue;

            types::Direction dir = dir_gen_.generate();
            Position newPos = it->second.GetPosition() + positionOffsetFromDirection(dir);
            if (checker_.isAvailable(newPos)) {
                monsters_.moveEntity(it->first, dir, newPos);
            }
        }
    }

    // Have a random number of monsters attack the closest player within their radius.
    void performRandomAttacks(uint32_t attackPower, size_t maxAttacks) {
        auto& monsterMap = monsters_.getEntities();
        if (monsterMap.empty())
            return;

        size_t attacks = std::min(maxAttacks, monsterMap.size());
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, monsterMap.size() - 1);

        for (size_t i = 0; i < attacks; ++i) {
            size_t idx = dist(gen);
            auto it = std::next(monsterMap.begin(), idx);
            if (!it->second.isAlive())
                continue;

            const auto& monster = it->second;
            auto targetId = monsters_.findClosestTarget(monster.GetPosition(), monster.getRadiusAttack(), players_);
            if (targetId) {
                auto* targetPlayer = players_.getEntity(*targetId);
                if (targetPlayer && targetPlayer->isAlive()) {
                    targetPlayer->damage(attackPower);
                }
            }
        }
    }

private:
    EntityManager<MonsterEntity, MobId>& monsters_;
    EntityManager<PlayerEntity, PlayerId>& players_;
    const CollisionChecker& checker_;
    DirectionGenerator& dir_gen_;
};

}  // namespace dungeons::server::domain
