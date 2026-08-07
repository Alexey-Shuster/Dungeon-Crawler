#pragma once

#include <cmath>
#include <compare>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

namespace map {
struct Position {
    using Dimension = int64_t;
    using Distance = uint64_t;

    constexpr explicit Position(Dimension x, Dimension y) : x{x}, y{y} {}

    [[nodiscard]] constexpr auto operator<=>(const Position& other) const = default;

    [[nodiscard]] constexpr Position operator-(const Position& other) const noexcept {
        return Position{sub(x, other.x), sub(y, other.y)};
    }

    [[nodiscard]] constexpr Position operator+(const Position& other) const noexcept {
        return Position{add(x, other.x), add(y, other.y)};
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
        return Position{x == std::numeric_limits<Dimension>::min() ? std::numeric_limits<Dimension>::max() : -x,
                        y == std::numeric_limits<Dimension>::min() ? std::numeric_limits<Dimension>::max() : -y};
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
    static constexpr std::make_unsigned_t<T> delta(T a, T b) noexcept {
        using U = std::make_unsigned_t<T>;
        return a >= b ? static_cast<U>(a) - static_cast<U>(b) : static_cast<U>(b) - static_cast<U>(a);
    }
    template <typename T>
    static constexpr T add(T a, T b) noexcept {
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
    static constexpr T sub(T a, T b) noexcept {
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
    static constexpr T mul(T a, T b) noexcept {
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
};

struct PositionHash {
    std::size_t operator()(const Position& position) const noexcept {
        auto h1 = std::hash<Position::Dimension>{}(position.x);
        auto h2 = std::hash<Position::Dimension>{}(position.y);
        return h1 ^ (h2 << 1);
    }
};

}  // namespace map
