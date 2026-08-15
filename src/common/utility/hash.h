#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace dungeons::common::utility {

// Allowed characters (alphanumeric)
static constexpr std::string_view kCharset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// Maximum number of characters that can be packed into 64 bits.
// 10 chars * 6 bits = 60 bits, leaving 4 bits for length.
static constexpr size_t kMaxPackedLen = 10;
static constexpr uint64_t kLengthMask = 0xFULL;  // 4 bits for length
static constexpr uint64_t kLengthShift = 60;     // bits 60..63
static constexpr uint64_t kCharMask = 0x3FULL;   // 6 bits per character

/// @brief Encodes an alphanumeric string into a 64‑bit integer.
/// @param str Input string (must be <= MAX_PACKED_LEN and only contain charset)
/// @return The packed integer, or std::nullopt if input is invalid.
[[nodiscard]] inline constexpr std::optional<uint64_t> EncodeString(std::string_view str) noexcept {
    if (str.length() > kMaxPackedLen) {
        return std::nullopt;
    }

    uint64_t code = (str.length()) << kLengthShift;
    size_t pos = 0;
    for (char c : str) {
        // Find character index in charset (linear search, fine for 62 chars)
        size_t idx = kCharset.find(c);
        if (idx == std::string_view::npos) {
            return std::nullopt;
        }
        code |= idx << (6 * pos);
        ++pos;
    }
    return code;
}

/// @brief Decodes a packed integer back to the original string.
/// @param code The 64‑bit packed value.
/// @return The original string, or std::nullopt if the code is malformed.
[[nodiscard]] inline constexpr std::optional<std::string> DecodeString(uint64_t code) noexcept {
    auto len = ((code >> kLengthShift) & kLengthMask);
    if (len > kMaxPackedLen) {
        return std::nullopt;
    }

    std::string result;
    result.reserve(len);

    for (size_t i = 0; i < len; ++i) {
        uint64_t idx = (code >> (6 * i)) & kCharMask;
        if (idx >= kCharset.length()) {
            // idx out of range (0..61)
            return std::nullopt;
        }
        result.push_back(kCharset[idx]);
    }

    // Optional: verify that all higher bits (above len*6) are zero,
    // except the length field (already checked).
    uint64_t used_bits = (len * 6);
    uint64_t unused_mask = (~uint64_t{0}) << used_bits;
    // But we also need to ignore the length bits (bits 60..63).
    // So mask out length bits.
    uint64_t length_bits = kLengthMask << kLengthShift;
    uint64_t check_mask = unused_mask & ~length_bits;
    if ((code & check_mask) != 0) {
        return std::nullopt;  // stray bits in unused area
    }

    return result;
}

struct Int64Hasher {
    [[nodiscard]] std::size_t operator()(uint64_t x) const noexcept {
        static constexpr uint64_t kMixerConstant1 = 0xbf58476d1ce4e5b9ULL;
        static constexpr uint64_t kMixerConstant2 = 0x94d049bb133111ebULL;

        x ^= x >> 30;
        x *= kMixerConstant1;
        x ^= x >> 27;
        x *= kMixerConstant2;
        x ^= x >> 31;

        return x;
    }
};

}  // namespace dungeons::common::utility
