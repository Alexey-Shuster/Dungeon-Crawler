#include "dungeon.h"

#include <common/network/game_state_dto.h>
#include <ranges>
#include <vector>

#include "create_game_state.h"

namespace dungeons::server::domain {

void Dungeon::addPlayerAttackCommand(PlayerId player_id, uint32_t damage) {
    command_queue_.push([self = weak_from_this(), player_id, damage]() {
        auto shared = self.lock();
        if (!shared)
            return;
        std::lock_guard lock(shared->entities_mutex_);

        auto* player = shared->players_.getEntity(player_id);
        if (!player || !player->isAlive())
            return;

        auto targetId =
            shared->players_.findClosestTarget(player->GetPosition(), player->getRadiusAttack(), shared->monsters_);
        if (targetId) {
            auto* monster = shared->monsters_.getEntity(*targetId);
            if (monster)
                monster->damage(damage);
        }
    });
}

void Dungeon::addMovePlayerCommand(PlayerId player_id, common::types::Direction direction) {
    command_queue_.push([self = weak_from_this(), player_id, direction]() {
        auto shared = self.lock();
        if (!shared)
            return;
        std::lock_guard lock(shared->entities_mutex_);

        auto* player = shared->players_.getEntity(player_id);
        if (!player || !player->isAlive())
            return;

        Position newPos = player->GetPosition() + positionOffsetFromDirection(direction);
        if (shared->collision_checker_.isAvailable(newPos)) {
            shared->players_.moveEntity(player_id, direction, newPos);
        }
    });
}

std::shared_ptr<common::network::DungeonSnapshot> Dungeon::processTick(std::chrono::milliseconds /*time_delta*/) {
    // Process player commands
    auto commands = command_queue_.popAll();
    while (!commands.empty()) {
        commands.front()();
        commands.pop();
    }

    // AI: monster movement and attack
    {
        std::lock_guard<std::mutex> lock(entities_mutex_);

        // Determine how many monsters to move/attack (randomly between 0 and size-1)
        size_t monsterCount = monsters_.getEntities().size();
        if (monsterCount > 0) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, monsterCount - 1);
            size_t moveCount = dist(gen);
            size_t attackCount = dist(gen);

            monster_ai_.moveRandomMonsters(moveCount);
            uint32_t attackPower = common::utility::getSettings().gameplay.monster_default_attack;
            monster_ai_.performRandomAttacks(attackPower, attackCount);
        }
    }

    // Check game over
    if (isGameOver()) {
        return nullptr;
    }

    // Build state
    DungeonState state;
    state.game_map = game_map_;
    state.players = players_.getEntities();
    state.monsters = monsters_.getEntities();

    return std::make_shared<common::network::DungeonSnapshot>(createGameStateDTO(state));
}

std::vector<PlayerId> Dungeon::getPlayers() const {
    std::vector<PlayerId> ids;
    for (const auto& id : players_.getEntities() | std::views::keys) {
        ids.push_back(id);
    }
    return ids;
}

bool Dungeon::isGameOver() const {
    for (const auto& player : players_.getEntities() | std::views::values) {
        if (player.isAlive()) {
            return false;
        }
    }
    return true;
}

}  // namespace dungeons::server::domain
