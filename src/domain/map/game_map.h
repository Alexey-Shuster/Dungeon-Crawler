#pragma once

#include <format>
#include <unordered_set>

#include "../../common/config.h"
#include "../../common/logger.h"
#include "map_size.h"
#include "position.h"

namespace map {

inline MapSize getCfgMapSize() {
    const auto& cfg = config::getSettings();
    return MapSize{
        Position(cfg.gameplay.default_map_blc_x, cfg.gameplay.default_map_blc_y),
        Position(cfg.gameplay.default_map_trc_x, cfg.gameplay.default_map_trc_y)
    };
}

class GameMap {
public:
    explicit GameMap(MapSize map_size = getCfgMapSize()) : map_size_{map_size} {
        LOG_INFO(std::format("GameMap created with bottom_left_corner ({}, {}), top_right_corner ({}, {})",
                             map_size_.getBottomLeftCorner().x,
                             map_size_.getBottomLeftCorner().y,
                             map_size_.getTopRightCorner().x,
                             map_size_.getTopRightCorner().y));
    }

    [[nodiscard]] bool addBarrier(const Position& position);

    [[nodiscard]] bool removeBarrier(const Position& position) noexcept;

    [[nodiscard]] bool isBarrier(const Position& position) const noexcept;

    [[nodiscard]] bool isInMap(const Position& position) const noexcept;

    [[nodiscard]] bool isAvailable(const Position& position) const noexcept;

    [[nodiscard]] const MapSize& size() const noexcept;

    [[nodiscard]] const std::unordered_set<Position, PositionHash>& getBarriers() const;

private:
    MapSize map_size_;
    std::unordered_set<Position, PositionHash> barriers_;
};

}  // namespace map
