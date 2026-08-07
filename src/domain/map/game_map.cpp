#include "game_map.h"

#include <format>

#include "../../common/logger.h"

bool map::GameMap::addBarrier(const Position& position) {
    if (!isInMap(position)) {
        LOG_ERROR(std::format("Cannot add barrier at position ({}, {}): position is outside map boundaries",
                              position.x,
                              position.y));
        return false;
    }
    if (!isBarrier(position)) {
        LOG_ERROR(
            std::format("Cannot add barrier at position ({}, {}): barrier already exists", position.x, position.y));
        return false;
    }
    auto [it, inserted] = barriers_.emplace(position);
    if (inserted) {
        LOG_INFO(std::format("Barrier added at position ({}, {})", position.x, position.y));
        return true;
    } else {
        LOG_ERROR(std::format("Barrier already exists at position ({}, {}), addition skipped", position.x, position.y));
        return false;
    }
}

bool map::GameMap::removeBarrier(const Position& position) noexcept {
    if (barriers_.erase(position)) {
        LOG_INFO(std::format("Barrier removed at position ({}, {})", position.x, position.y));
        return true;
    }
    LOG_ERROR(std::format("Barrier not found at position ({}, {})", position.x, position.y));
    return false;
}

bool map::GameMap::isBarrier(const Position& position) const noexcept {
    return barriers_.contains(position);
}

bool map::GameMap::isInMap(const Position& position) const noexcept {
    return map_size_.isInMap(position);
}

bool map::GameMap::isAvailable(const Position& position) const noexcept {
    return isInMap(position) && !isBarrier(position);
}

const map::MapSize& map::GameMap::size() const noexcept {
    return map_size_;
}
const std::unordered_set<map::Position, map::PositionHash>& map::GameMap::getBarriers() const {
    return barriers_;
}
