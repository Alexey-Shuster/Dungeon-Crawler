#pragma once

namespace dungeons::common::utility {

template <typename T>
constexpr bool isBetween(const T& value, const T& low, const T& high) {
    return value >= low && value <= high;
}

}  // namespace dungeons::common::utility
