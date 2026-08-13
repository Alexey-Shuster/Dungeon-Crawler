#pragma once

#include <cmath>
#include <compare>  // IWYU pragma: keep // operator<=>
#include <cstdint>  // IWYU pragma: keep // uint64_t
#include <functional>
#include <limits>
#include <type_traits>

#include "direction.h"

namespace dungeons::server::domain {

struct Position {
    using Dimension = int64_t;
    using Distance = uint64_t;

    constexpr explicit Position(Dimension x, Dimension y)
        : x{x}
        , y{y} {}

    [[nodiscard]] constexpr auto operator<=>(const Position& other) const = default;

    [[nodiscard]] constexpr Position operator-(Position other) const noexcept {
        Position result = *this;
        result -= other;
        return result;
    }

    [[nodiscard]] constexpr Position operator+(Position other) const noexcept {
        other += *this;
        return other;
    }

    constexpr Position& operator-=(const Position& other) noexcept {
        x = sub(x, other.x);
        y = sub(y, other.y);
        return *this;
    }

    constexpr Position& operator+=(const Position& other) noexcept {
        x = add(x, other.x);
        y = add(y, other.y);
        return *this;
    }

    [[nodiscard]] constexpr Position operator-() const noexcept {
        return Position{sub(Dimension{0}, x), sub(Dimension{0}, y)};
    }

    Distance manhattanDistance(const Position& other) const noexcept {
        auto dx = delta(other.x, x);
        auto dy = delta(other.y, y);
        return add(dx, dy);
    }

    Distance euclideanDistance(const Position& other) const noexcept {
        auto dx = delta(other.x, x);
        auto dy = delta(other.y, y);
        return std::sqrt(add(mul(dx, dx), mul(dy, dy)));
    }

    Dimension x{};
    Dimension y{};

private:
    template <typename T>
    static constexpr std::make_unsigned_t<T> delta(T a, T b) noexcept;

    template <typename T>
    static constexpr T add(T a, T b) noexcept;

    template <typename T>
    static constexpr T sub(T a, T b) noexcept;

    template <typename T>
    static constexpr T mul(T a, T b) noexcept;
};

template <typename T>
constexpr std::make_unsigned_t<T> Position::delta(T a, T b) noexcept {
    using U = std::make_unsigned_t<T>;
    return a >= b ? static_cast<U>(a) - static_cast<U>(b) : static_cast<U>(b) - static_cast<U>(a);
}

template <typename T>
constexpr T Position::add(T a, T b) noexcept {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    if (b > 0 && a > std::numeric_limits<T>::max() - b) {
        return std::numeric_limits<T>::max();
    }
    if (b < 0 && a < std::numeric_limits<T>::min() - b) {
        return std::numeric_limits<T>::min();
    }
    return a + b;
}

template <typename T>
constexpr T Position::sub(T a, T b) noexcept {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    if (b > 0 && a < std::numeric_limits<T>::min() + b) {
        return std::numeric_limits<T>::min();
    }
    if (b < 0 && a > std::numeric_limits<T>::max() + b) {
        return std::numeric_limits<T>::max();
    }
    return a - b;
}

template <typename T>
constexpr T Position::mul(T a, T b) noexcept {
    static_assert(std::is_integral<T>::value, "T must be integral type");
    if (a == 0 || b == 0) {
        return 0;
    }
    if constexpr (std::is_unsigned<T>::value) {
        if (a > std::numeric_limits<T>::max() / b) {
            return std::numeric_limits<T>::max();
        }
        return a * b;
    } else {
        if (a > 0 && b > 0) {
            if (a > std::numeric_limits<T>::max() / b) {
                return std::numeric_limits<T>::max();
            }
        } else if (a < 0 && b < 0) {
            if (a < std::numeric_limits<T>::max() / b) {
                return std::numeric_limits<T>::max();
            }
        } else if (a > 0 && b < 0) {
            if (b < std::numeric_limits<T>::min() / a) {
                return std::numeric_limits<T>::min();
            }
        } else if (a < 0 && b > 0) {
            if (a < std::numeric_limits<T>::min() / b) {
                return std::numeric_limits<T>::min();
            }
        }
        return a * b;
    }
}

struct PositionHash {
    std::size_t operator()(const Position& position) const noexcept {
        auto h1 = std::hash<Position::Dimension>{}(position.x);
        auto h2 = std::hash<Position::Dimension>{}(position.y);
        return h1 ^ (h2 << 1);
    }
};

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

}  // namespace dungeons::server::domain
