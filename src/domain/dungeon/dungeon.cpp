#include "dungeon.h"

#include <random>
#include <ranges>
#include <vector>

#include "../../common/logger.h"

void dungeon::Dungeon::updateMonstersCounterDistributions() {
    dist_monsters_counter_ =
        std::uniform_int_distribution<size_t>{0, monsters_entities_.size() == 0 ? 0 : monsters_entities_.size() - 1};
}

map::Position dungeon::Dungeon::makeRandomPosition() const noexcept {
    return map::Position{dist_x_(gen_), dist_y_(gen_)};
}

Direction dungeon::Dungeon::makeRandomDirection() const noexcept {
    return static_cast<Direction>(dist_direction_(gen_));
}

void dungeon::Dungeon::moveRandomMonsters() {
    if (monsters_entities_.empty()) {
        return;
    }
    size_t monsters_counters = dist_monsters_counter_(gen_);
    for (size_t i = 0; i < monsters_counters; ++i) {
        size_t monster_number = dist_monsters_counter_(gen_);
        auto it = std::next(monsters_entities_.begin(), monster_number);
        if (!(it->second.isAlive())) {
            continue;
        }
        auto direction = makeRandomDirection();
        moveEntity(monsters_entities_, it->first, direction);
    }
}

void dungeon::Dungeon::monstersRandomAttack() {
    if (monsters_entities_.empty()) {
        return;
    }
    size_t monsters_counters = dist_monsters_counter_(gen_);
    for (size_t i = 0; i < monsters_counters; ++i) {
        size_t monster_number = dist_monsters_counter_(gen_);
        auto it = std::next(monsters_entities_.begin(), monster_number);
        attackByEntity(monsters_entities_, it->first, players_entities_, 10);
    }
}

void dungeon::Dungeon::addPlayerAttackCommand(PlayerId player_id, uint32_t damage) {
    std::lock_guard<std::mutex> guard(players_action_mtx_);
    commands_.push([self = weak_from_this(), player_id, damage]() {
        auto shared_self = self.lock();
        shared_self->attackByEntity(shared_self->players_entities_, player_id, shared_self->monsters_entities_, damage);
    });
}

void dungeon::Dungeon::addMovePlayerCommand(PlayerId player_id, Direction direction) {
    std::lock_guard<std::mutex> guard(players_action_mtx_);
    commands_.push([self = weak_from_this(), player_id, direction]() {
        auto shared_self = self.lock();
        shared_self->moveEntity(shared_self->players_entities_, player_id, direction);
    });
}

std::optional<dungeon::DungeonState> dungeon::Dungeon::processTick(
    [[maybe_unused]] std::chrono::milliseconds time_delta) {
    std::queue<std::function<void()>> local_commands;
    {
        std::lock_guard<std::mutex> guard(players_action_mtx_);
        local_commands.swap(commands_);
    }
    std::lock_guard<std::mutex> guard(entities_container_mtx_);
    while (!local_commands.empty()) {
        local_commands.front()();
        local_commands.pop();
    }
    moveRandomMonsters();
    monstersRandomAttack();

    if (isGameOver()) {
        return std::nullopt;
    }

    auto dungeon_state = DungeonState{game_map_, players_entities_, monsters_entities_};
    return std::optional<DungeonState>{std::move(dungeon_state)};
}

std::vector<PlayerId> dungeon::Dungeon::getPlayers() const {
    std::vector<PlayerId> player_ids;
    for (const auto& id : players_entities_ | std::views::keys) {
        player_ids.push_back(id);
    }
    return player_ids;
}

bool dungeon::Dungeon::addPlayerEntity(PlayerId player_id) {
    return addEntity(players_entities_, player_id);
}

bool dungeon::Dungeon::addMonsterEntity(MobId mob_id) {
    return addEntity(monsters_entities_, mob_id);
}

bool dungeon::Dungeon::isAvailable(const map::Position& position) const noexcept {
    return game_map_.isAvailable(position) && !isOccupiedByEntity(players_entities_, position) &&
           !isOccupiedByEntity(monsters_entities_, position);
}

bool dungeon::Dungeon::isGameOver() const noexcept {
    for (const auto& player_entity : players_entities_ | std::views::values) {
        if (player_entity.isAlive()) {
            return false;
        }
    }
    return true;
}
