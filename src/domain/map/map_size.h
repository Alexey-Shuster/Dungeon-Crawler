#pragma once

#include <cstdint>

#include "position.h"

namespace map {

struct MapSize {
    using Dimension = Position::Dimension;

    constexpr explicit MapSize(Position blc, Position trc) : bottom_left_corner_{blc}, top_right_corner_{trc} {
        assert(isCorrect() && "Invalid map size");
    }

    [[nodiscard]] constexpr bool isInMap(const Position& pos) const noexcept {
        return (pos.x >= bottom_left_corner_.x) && (pos.y >= bottom_left_corner_.y) && (pos.x <= top_right_corner_.x) &&
               (pos.y <= top_right_corner_.y);
    }

    [[nodiscard]] const Position& getBottomLeftCorner() const noexcept {
        return bottom_left_corner_;
    }

    [[nodiscard]] const Position& getTopRightCorner() const noexcept {
        return top_right_corner_;
    }

private:
    [[nodiscard]] constexpr bool isCorrect() const noexcept {
        return (bottom_left_corner_.x < top_right_corner_.x) && (bottom_left_corner_.y < top_right_corner_.y);
    }

private:
    Position bottom_left_corner_;
    Position top_right_corner_;
};

}  // namespace map
