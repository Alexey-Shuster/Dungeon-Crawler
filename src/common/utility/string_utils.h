#pragma once

#include <charconv>
#include <format>
#include <string>

namespace dungeons::common::utility {

[[nodiscard]] constexpr std::string_view trimWhitespace(std::string_view str) noexcept {
    constexpr std::string_view whitespace = " \t\n\r\f\v";

    const auto start = str.find_first_not_of(whitespace);
    if (start == std::string_view::npos) {
        return {};
    }

    const auto end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// Joins any number of args into string with a space
template <typename... Args>
std::string joinWithSpace(const Args... args) {
    if constexpr (sizeof...(Args) == 0) {
        return "";
    }

    std::string result;

    ((result.empty() ? std::format_to(std::back_inserter(result), "{}", args)
                     : std::format_to(std::back_inserter(result), " {}", args)),
     ...);

    return result;
}

// Safely converts string into Uint64_t
[[nodiscard]] inline std::optional<uint64_t> convertStringToUint64(std::string_view num_str) noexcept {
    auto num = trimWhitespace(num_str);

    if (num.empty()) {
        return std::nullopt;
    }

    // Parse the string directly into uint64_t
    uint64_t value = 0;
    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), value);

    // Check for parsing success and ensure it consumed the whole string
    if (ec == std::errc{} && ptr == num.data() + num.size()) {
        return value;
    }

    return std::nullopt;
}

}  // namespace dungeons::common::utility
