#pragma once

#include <atomic>
#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>

#include "../../common/config.h"
#include "../../common/direction.h"
#include "../../common/logger.h"
#include "../../common/types.h"
#include "../entity/entity.h"
#include "../map/game_map.h"
#include "../map/position.h"

namespace dungeon {

constexpr size_t kMaxAttempts{1000};

struct DungeonState {
    map::GameMap game_map;
    std::unordered_map<PlayerId, entity::PlayerEntity, StrongIdHash<PlayerId>> players;
    std::unordered_map<MobId, entity::MonsterEntity, StrongIdHash<MobId>> monsters;
};

class Dungeon : public std::enable_shared_from_this<Dungeon> {
public:
    template <typename PlayersContainer>
    explicit Dungeon(map::GameMap game_map, PlayersContainer&& container)
        : game_map_{game_map}
        , dist_x_{game_map_.size().getBottomLeftCorner().x, game_map_.size().getTopRightCorner().x}
        , dist_y_{game_map_.size().getBottomLeftCorner().y, game_map_.size().getTopRightCorner().y}
        , dist_direction_{0, 3}
        , dist_monsters_counter_{0, 0} {
        for (auto& player_id : container) {
            addPlayerEntity(player_id);
        }
        for (size_t j = 0; j < std::size(container); ++j) {
            for (size_t i = 0; i < config::getSettings().gameplay.monsters_per_player; ++i) {
                addMonsterEntity(MobId{mob_id_counter_});
                ++mob_id_counter_;
            }
        }

        updateMonstersCounterDistributions();
    }

    void addPlayerAttackCommand(PlayerId player_id, uint32_t damage = 10);
    void addMovePlayerCommand(PlayerId player_id, Direction direction);
    std::optional<DungeonState> processTick([[maybe_unused]] std::chrono::milliseconds time_delta);
    std::vector<PlayerId> getPlayers() const;

protected:
    template <typename Container>
    static auto getEntityPtr(const Container& entity_container, const Container::key_type& id)
        -> const Container::mapped_type* {
        auto it = entity_container.find(id);
        if (it == entity_container.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    template <typename Container>
    static auto getEntityPtr(Container& container, const Container::key_type& id) -> Container::mapped_type* {
        auto it = container.find(id);
        if (it == container.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    template <typename Container>
    bool addEntity(Container& container, const Container::key_type& id) {
        if (container.find(id) != container.end()) {
            LOG_ERROR(std::format("Entity with id {} already exists", id.value));
            return false;
        }

        size_t attempts = 0;
        auto position = makeRandomPosition();

        while (!isAvailable(position) && attempts < kMaxAttempts) {
            LOG_INFO(std::format("Position ({}, {}) is not available for entity {}", position.x, position.y, id.value));
            ++attempts;
            LOG_INFO(std::format("Searching free pos for entity {}: attempt {}/{}", id.value, attempts, kMaxAttempts));
            position = makeRandomPosition();
        }

        if (!isAvailable(position)) {
            LOG_ERROR(std::format("Failed to add entity id {}: no free position found after {} attempts",
                                  id.value,
                                  kMaxAttempts));
            return false;
        }

        typename Container::mapped_type entity{id, position};
        auto [it, inserted] = container.insert({id, std::move(entity)});
        if (inserted) {
            LOG_INFO(std::format("Entity id {} placed at pos ({}, {}) after {} attempts",
                                 id.value,
                                 position.x,
                                 position.y,
                                 attempts + 1));
        } else {
            LOG_ERROR(std::format("Unexpected: entity {} already exists in container!", id.value));
        }
        return inserted;
    }

    template <typename Container>
    bool moveEntity(Container& container, const Container::key_type& id, Direction direction) {
        auto entity_ptr = getEntityPtr(container, id);
        if (!entity_ptr) {
            return false;
        }
        auto offset = map::positionOffsetFromDirection(direction);
        auto new_position = entity_ptr->GetPosition() + offset;
        if (!isAvailable(new_position)) {
            return false;
        }
        entity_ptr->SetPosition(new_position);
        return true;
    }

    template <typename Container>
    bool isOccupiedByEntity(const Container& container, const map::Position& position) const noexcept {
        for (const auto& [entity_id, entity] : container) {
            if (entity.GetPosition() == position) {
                return true;
            }
        }
        return false;
    }

    template <typename Container, typename TargetsContainer>
    void attackByEntity(const Container& container,
                        const Container::key_type& id,
                        TargetsContainer& targets_container,
                        uint32_t damage) {
        auto entity_ptr = getEntityPtr(container, id);
        if (!entity_ptr || !(entity_ptr->isAlive())) {
            return;
        }
        auto atack_position = entity_ptr->GetPosition();
        auto radius_attack = entity_ptr->getRadiusAttack();
        auto target_id = findEntityTarget(targets_container, atack_position, radius_attack);
        if (!target_id.has_value()) {
            return;
        }
        auto target_ptr = getEntityPtr(targets_container, *target_id);
        if (!target_ptr) {
            return;
        }
        target_ptr->damage(damage);
    }

    template <typename TargetsContainer>
    std::optional<typename TargetsContainer::key_type> findEntityTarget(TargetsContainer& targets_container,
                                                                        const map::Position& atack_position,
                                                                        uint64_t radius_attack) const noexcept {
        std::optional<typename TargetsContainer::key_type> target_id;
        uint64_t min_distance = radius_attack;
        for (const auto& [possible_target_id, possible_target] : targets_container) {
            const auto& possible_target_position = possible_target.GetPosition();
            auto distance_to_possible_target = atack_position.manhattanDistance(possible_target_position);
            if (distance_to_possible_target <= min_distance) {
                min_distance = distance_to_possible_target;
                target_id = possible_target_id;
            }
        }
        return target_id;
    }

    void updateMonstersCounterDistributions();
    map::Position makeRandomPosition() const noexcept;
    Direction makeRandomDirection() const noexcept;
    void moveRandomMonsters();
    void monstersRandomAttack();
    bool addPlayerEntity(PlayerId player_id);
    bool addMonsterEntity(MobId mob_id);
    bool isAvailable(const map::Position& position) const noexcept;
    bool isGameOver() const noexcept;

protected:
    std::unordered_map<PlayerId, entity::PlayerEntity, StrongIdHash<PlayerId>> players_entities_;
    std::unordered_map<MobId, entity::MonsterEntity, StrongIdHash<MobId>> monsters_entities_;
    map::GameMap game_map_;
    std::queue<std::function<void()>> commands_;
    mutable std::mutex players_action_mtx_;
    mutable std::mutex entities_container_mtx_;

    std::atomic<size_t> mob_id_counter_{0};
    std::random_device rd_;
    mutable std::mt19937 gen_{rd_()};
    mutable std::uniform_int_distribution<map::Position::Dimension> dist_x_;
    mutable std::uniform_int_distribution<map::Position::Dimension> dist_y_;
    mutable std::uniform_int_distribution<size_t> dist_direction_;
    mutable std::uniform_int_distribution<size_t> dist_monsters_counter_;
};

}  // namespace dungeon
