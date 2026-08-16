#pragma once

#include <random>

#include "map/game_map.h"
#include "map/position.h"

namespace dungeons::server::domain {

class PositionGenerator {
public:
    explicit PositionGenerator(const GameMap& map)
        : gen_(std::random_device{}())
        , distX_(map.size().getBottomLeftCorner().x, map.size().getTopRightCorner().x)
        , distY_(map.size().getBottomLeftCorner().y, map.size().getTopRightCorner().y) {}

    Position generate() const {
        return Position{distX_(gen_), distY_(gen_)};
    }

private:
    mutable std::mt19937 gen_;
    mutable std::uniform_int_distribution<Position::Dimension> distX_;
    mutable std::uniform_int_distribution<Position::Dimension> distY_;
};

}  // namespace dungeons::server::domain
