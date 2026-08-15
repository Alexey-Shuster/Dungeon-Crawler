#include "game_map.h"

#include <common/utility/logger.h>
#include <format>

namespace dungeons::server::domain {

GameMap::GameMap(MapSize map_size)
    : map_size_{map_size} {
    LOG_INFO(std::format("GameMap created with bottom_left_corner ({}, {}), top_right_corner ({}, {})",
                         map_size_.getBottomLeftCorner().x,
                         map_size_.getBottomLeftCorner().y,
                         map_size_.getTopRightCorner().x,
                         map_size_.getTopRightCorner().y));
}

bool GameMap::addBarrier(const Position& position) {
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

bool GameMap::removeBarrier(const Position& position) noexcept {
    if (barriers_.erase(position)) {
        LOG_INFO(std::format("Barrier removed at position ({}, {})", position.x, position.y));
        return true;
    }
    LOG_ERROR(std::format("Barrier not found at position ({}, {})", position.x, position.y));
    return false;
}

bool GameMap::isBarrier(const Position& position) const noexcept {
    return barriers_.contains(position);
}

bool GameMap::isInMap(const Position& position) const noexcept {
    return map_size_.isInMap(position);
}

bool GameMap::isAvailable(const Position& position) const noexcept {
    return isInMap(position) && !isBarrier(position);
}

const MapSize& GameMap::size() const noexcept {
    return map_size_;
}
const std::unordered_set<Position, PositionHash>& GameMap::getBarriers() const {
    return barriers_;
}

}  // namespace dungeons::server::domain
