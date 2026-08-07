#pragma once

#include <array>
#include <cassert>
#include <string>

#include "../../common/direction.h"
#include "position.h"

namespace map {

enum class Direction { kUp, kDown, kLeft, kRight };

[[nodiscard]] inline constexpr Position positionOffsetFromDirection(Direction direction) noexcept {
    switch (direction) {
        case Direction::kUp:
            return Position{0, 1};
        case Direction::kDown:
            return Position{0, -1};
        case Direction::kLeft:
            return Position{-1, 0};
        case Direction::kRight:
            return Position{1, 0};
    }
    return Position{0, 0};
}

[[nodiscard]] inline constexpr Direction commandDirectionFromMessageDirection(::Direction direction) noexcept {
    switch (direction) {
        case ::Direction::kUp:
            return Direction::kDown;
        case ::Direction::kDown:
            return Direction::kUp;
        case ::Direction::kLeft:
            return Direction::kLeft;
        case ::Direction::kRight:
            return Direction::kRight;
    }
    return Direction::kUp;
}

[[nodiscard]] inline constexpr std::string directionToString(Direction direction) {
    switch (direction) {
        case Direction::kUp:
            return "Up";
        case Direction::kDown:
            return "Down";
        case Direction::kLeft:
            return "Left";
        case Direction::kRight:
            return "Right";
    }
    assert(false);
}

}  // namespace map
