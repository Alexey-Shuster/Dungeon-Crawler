#pragma once

#include <common/utility/config.h>
#include <unordered_set>

#include "map_size.h"
#include "position.h"

namespace dungeons::server::domain {

inline MapSize getCfgMapSize() {
    const auto& cfg = common::utility::getSettings();
    return MapSize{Position(cfg.gameplay.default_map_blc_x, cfg.gameplay.default_map_blc_y),
                   Position(cfg.gameplay.default_map_trc_x, cfg.gameplay.default_map_trc_y)};
}

class GameMap {
public:
    explicit GameMap(MapSize map_size = getCfgMapSize());

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

}  // namespace dungeons::server::domain
