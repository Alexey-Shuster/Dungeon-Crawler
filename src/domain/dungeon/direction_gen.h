#pragma once

#include <random>
#include "common/direction.h"

namespace dungeon {

class DirectionGenerator {
public:
    DirectionGenerator()
        : gen_(std::random_device{}())
        , dist_(0, 3) {}

    Direction generate() const {
        return static_cast<Direction>(dist_(gen_));
    }

private:
    mutable std::mt19937 gen_;
    mutable std::uniform_int_distribution<size_t> dist_;
};

} // namespace dungeon
