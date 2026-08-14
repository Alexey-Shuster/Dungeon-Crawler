#pragma once

#include <cstdint>  // IWYU pragma: keep
#include <optional>
#include <string_view>

namespace types {

enum class Direction : uint8_t {
    kUp = 0,
    kDown,
    kLeft,
    kRight
};

[[nodiscard]] constexpr std::optional<std::string_view> directionToString(Direction dir) noexcept {
    switch (dir) {
        case Direction::kUp:
            return "UP";
        case Direction::kDown:
            return "DOWN";
        case Direction::kLeft:
            return "LEFT";
        case Direction::kRight:
            return "RIGHT";
    }
    return std::nullopt;
}

[[nodiscard]] constexpr std::optional<Direction> directionFromString(std::string_view str) noexcept {
    if (str == "UP")
        return Direction::kUp;
    if (str == "DOWN")
        return Direction::kDown;
    if (str == "LEFT")
        return Direction::kLeft;
    if (str == "RIGHT")
        return Direction::kRight;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<Direction> byteToDirection(uint8_t value) noexcept {
    switch (static_cast<Direction>(value)) {
        case Direction::kUp:
        case Direction::kDown:
        case Direction::kLeft:
        case Direction::kRight:
            return static_cast<Direction>(value);
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<uint8_t> directionToByte(Direction dir) noexcept {
    switch (dir) {
        case Direction::kUp:
        case Direction::kDown:
        case Direction::kLeft:
        case Direction::kRight:
            return static_cast<uint8_t>(dir);
        default:
            return std::nullopt;
    }

}

}  // namespace types
