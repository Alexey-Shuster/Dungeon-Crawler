#pragma once

#include <random>

#include "domain/map/game_map.h"
#include "domain/map/position.h"

namespace dungeon {

class PositionGenerator {
public:
    explicit PositionGenerator(const map::GameMap& map)
        : gen_(std::random_device{}())
        , distX_(map.size().getBottomLeftCorner().x, map.size().getTopRightCorner().x)
        , distY_(map.size().getBottomLeftCorner().y, map.size().getTopRightCorner().y) {}

    map::Position generate() const {
        return map::Position{distX_(gen_), distY_(gen_)};
    }

private:
    mutable std::mt19937 gen_;
    mutable std::uniform_int_distribution<map::Position::Dimension> distX_;
    mutable std::uniform_int_distribution<map::Position::Dimension> distY_;
};

}  // namespace dungeon
