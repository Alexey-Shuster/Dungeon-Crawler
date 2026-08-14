#pragma once

#include <format>
#include <optional>
#include <type_traits>

#include "logger.h"
#include "message.h"
#include "message_types.h"

namespace message {

// -----------------------------------------------------------------------------
// Packing constants – Level (2 bits) + id (8 bits) → 16‑bit header
// -----------------------------------------------------------------------------
static constexpr uint8_t kLevelBits = 2;
static constexpr uint8_t kIdBits = 8;
static constexpr uint8_t kLevelShift = kIdBits;
static constexpr uint8_t kIdShift = 0;

static constexpr uint16_t kLevelMask = (1 << kLevelBits) - 1;  // 0x3
static constexpr uint16_t kIdMask = (1 << kIdBits) - 1;        // 0xFF

static constexpr size_t kPackedTypeSize = sizeof(uint16_t);

// -----------------------------------------------------------------------------
// EnumTraits – map each concrete message enum to its Level
// -----------------------------------------------------------------------------
template <typename Enum>
struct EnumTraits;

template <>
struct EnumTraits<NetworkMessageType> {
    static constexpr Level level = Level::kNetwork;
};

template <>
struct EnumTraits<AppMessageType> {
    static constexpr Level level = Level::kApp;
};

template <>
struct EnumTraits<DomainMessageType> {
    static constexpr Level level = Level::kDomain;
};

// -----------------------------------------------------------------------------
// Convert Level enum ↔ uint8_t for packing
// -----------------------------------------------------------------------------
[[nodiscard]] inline std::optional<uint8_t> levelToUint8(Level lvl) noexcept {
    using Underlying = std::underlying_type_t<Level>;
    const auto raw = static_cast<Underlying>(lvl);

    // Check the raw value before any truncation
    if (raw < 0 || raw > static_cast<Underlying>(kLevelMask)) {
        LOG_ERROR(std::format("Level {} exceeds mask", raw));
        return std::nullopt;
    }
    return static_cast<uint8_t>(raw);
}

[[nodiscard]] inline std::optional<Level> uint8ToLevel(uint8_t raw) noexcept {
    using Underlying = std::underlying_type_t<Level>;
    // Compare raw against the valid range of the Level enum
    constexpr auto kMaxLevel = static_cast<Underlying>(Level::kUnknown);
    if (raw > static_cast<uint8_t>(kMaxLevel)) {
        LOG_ERROR(std::format("Invalid level value {}", raw));
        return std::nullopt;
    }
    return static_cast<Level>(raw);
}

// -----------------------------------------------------------------------------
// Extract (Level, id) from a MessageTypeVariant
// -----------------------------------------------------------------------------
struct LevelId {
    Level level;
    uint8_t id;
};

template <typename>
struct always_false : std::false_type {};

[[nodiscard]] inline std::optional<LevelId> getLevelId(const MessageTypeVariant& v) noexcept {
    using namespace message;
    return std::visit(
        []<typename T0>(T0&& arg) -> std::optional<LevelId> {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, NetworkMessageType>) {
                return LevelId{Level::kNetwork, static_cast<uint8_t>(arg)};
            } else if constexpr (std::is_same_v<T, AppMessageType>) {
                return LevelId{Level::kApp, static_cast<uint8_t>(arg)};
            } else if constexpr (std::is_same_v<T, DomainMessageType>) {
                return LevelId{Level::kDomain, static_cast<uint8_t>(arg)};
            } else {
                static_assert(always_false<T>::value, "Unexpected type in variant");
                return std::nullopt;
            }
        },
        v);
}

// -----------------------------------------------------------------------------
// Reconstruct MessageTypeVariant from (Level, id)
// -----------------------------------------------------------------------------
[[nodiscard]] inline std::optional<MessageTypeVariant> makeVariantFromLevelId(Level level, uint8_t id) noexcept {
    using namespace message;
    switch (level) {
        case Level::kNetwork:
            if (id >= static_cast<uint8_t>(NetworkMessageType::kMax)) {
                LOG_ERROR(std::format("Invalid Network ID {}", id));
                return std::nullopt;
            }
            return static_cast<NetworkMessageType>(id);

        case Level::kApp:
            if (id >= static_cast<uint8_t>(AppMessageType::kMax)) {
                LOG_ERROR(std::format("Invalid App ID {}", id));
                return std::nullopt;
            }
            return static_cast<AppMessageType>(id);

        case Level::kDomain:
            if (id >= static_cast<uint8_t>(DomainMessageType::kMax)) {
                LOG_ERROR(std::format("Invalid Domain ID {}", id));
                return std::nullopt;
            }
            return static_cast<DomainMessageType>(id);

        case Level::kUnknown:
        default:
            LOG_ERROR(std::format("Invalid or unknown Level {} Id {}", static_cast<uint8_t>(level), id));
            return std::nullopt;
    }
}

// -----------------------------------------------------------------------------
// Convenience queries (based on Level)
// -----------------------------------------------------------------------------
[[nodiscard]] inline bool isNetworkMessage(const MessageTypeVariant& v) noexcept {
    auto info = getLevelId(v);
    return info && info->level == Level::kNetwork;
}

[[nodiscard]] inline bool isAppMessage(const MessageTypeVariant& v) noexcept {
    auto info = getLevelId(v);
    return info && info->level == Level::kApp;
}

[[nodiscard]] inline bool isDomainMessage(const MessageTypeVariant& v) noexcept {
    auto info = getLevelId(v);
    return info && info->level == Level::kDomain;
}

[[nodiscard]] inline bool isUnknownMessage(const MessageTypeVariant& v) noexcept {
    return std::holds_alternative<std::monostate>(v);
}

// -----------------------------------------------------------------------------
// Specific accessors (return the concrete enum)
// -----------------------------------------------------------------------------
[[nodiscard]] inline std::optional<NetworkMessageType> asNetworkMessage(const MessageTypeVariant& v) noexcept {
    if (auto* p = std::get_if<NetworkMessageType>(&v))
        return *p;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<AppMessageType> asAppMessage(const MessageTypeVariant& v) noexcept {
    if (auto* p = std::get_if<AppMessageType>(&v))
        return *p;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<DomainMessageType> asDomainMessage(const MessageTypeVariant& v) noexcept {
    if (auto* p = std::get_if<DomainMessageType>(&v))
        return *p;
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// Pack / unpack (Level, id) ↔ uint16_t
// -----------------------------------------------------------------------------
[[nodiscard]] inline std::optional<uint16_t> packLevelId(Level level, uint8_t id) noexcept {
    auto lvlU8 = levelToUint8(level);
    if (!lvlU8)
        return std::nullopt;
    return (static_cast<uint16_t>(*lvlU8) << kLevelShift) | static_cast<uint16_t>(id);
}

[[nodiscard]] inline std::optional<uint16_t> packMessageType(const MessageTypeVariant& v) noexcept {
    auto info = getLevelId(v);
    if (!info)
        return std::nullopt;
    return packLevelId(info->level, info->id);
}

[[nodiscard]] inline std::optional<MessageTypeVariant> unpackMessageType(uint16_t code) noexcept {
    // Extract raw level without masking – validation done by uint8ToLevel
    const uint8_t lvlRaw = static_cast<uint8_t>(code >> kLevelShift);
    const uint8_t id = static_cast<uint8_t>(code & kIdMask);

    auto level = uint8ToLevel(lvlRaw);
    if (!level)
        return std::nullopt;

    return makeVariantFromLevelId(*level, id);
}

// -----------------------------------------------------------------------------
// Wire format helpers – big‑endian (network order)
// -----------------------------------------------------------------------------
inline void appendPackedType(uint16_t packed, std::vector<uint8_t>& out) {
    out.push_back(static_cast<uint8_t>(packed >> 8));    // high byte
    out.push_back(static_cast<uint8_t>(packed & 0xFF));  // low byte
}

[[nodiscard]] inline std::optional<uint16_t> readPackedType(const std::vector<uint8_t>& buffer,
                                                            size_t offset) noexcept {
    if (offset + kPackedTypeSize > buffer.size())
        return std::nullopt;
    return (static_cast<uint16_t>(buffer[offset]) << 8) | static_cast<uint16_t>(buffer[offset + 1]);
}

// -----------------------------------------------------------------------------
// makeMessage & parseMessage
// -----------------------------------------------------------------------------

[[nodiscard]] inline network::Message makeMessage(const MessageTypeVariant& type, network::ByteBuffer payload) {
    auto packed = packMessageType(type);
    if (!packed) {
        LOG_ERROR("Failed to pack message type – returning empty");
        return network::Message{std::vector<uint8_t>{}};
    }
    std::vector<uint8_t> data;
    data.reserve(kPackedTypeSize + payload.size());
    appendPackedType(*packed, data);
    data.insert(data.end(), payload.begin(), payload.end());
    return network::Message{std::move(data)};
}

struct DeserializedBuffer {
    MessageTypeVariant type;
    network::ByteBuffer payload;
};

[[nodiscard]] inline std::optional<DeserializedBuffer> parseMessage(const network::Message& msg) {
    const auto& data = msg.buffer;
    if (data.size() < kPackedTypeSize) {
        LOG_ERROR("Message too short to contain header");
        return std::nullopt;
    }

    auto packed = readPackedType(data, 0);
    if (!packed)
        return std::nullopt;

    auto type = unpackMessageType(*packed);
    if (!type) {
        LOG_ERROR(std::format("Failed to unpack type from code {:#04x}", *packed));
        return std::nullopt;
    }

    // Copy payload (excluding the header) into a new vector
    network::ByteBuffer payload(data.begin() + kPackedTypeSize, data.end());
    return DeserializedBuffer{*type, std::move(payload)};
}

}  // namespace message
