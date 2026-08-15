#pragma once

#include <format>
#include <network/raw_message.h>
#include <optional>
#include <types/message_types.h>
#include <types/message_utils.h>
#include <utility/logger.h>

namespace dungeons::common::wire {

[[nodiscard]] inline network::RawMessage makeMessage(const types::MessageTypeVariant& type,
                                                     network::ByteBuffer payload) {
    auto packed = packMessageType(type);
    if (!packed) {
        LOG_ERROR("Failed to pack message type – returning empty");
        return network::RawMessage{network::ByteBuffer{}};
    }
    std::vector<uint8_t> data;
    data.reserve(types::kPackedTypeSize + payload.size());
    types::appendPackedType(*packed, data);
    data.insert(data.end(), payload.begin(), payload.end());
    return network::RawMessage{std::move(data)};
}

struct ParsedRawMessage {
    types::MessageTypeVariant type;
    network::ByteBuffer payload;
};

[[nodiscard]] inline std::optional<ParsedRawMessage> parseMessage(network::RawMessage msg) {
    const auto& data = msg.buffer;
    if (data.size() < types::kPackedTypeSize) {
        LOG_ERROR("Message too short to contain header");
        return std::nullopt;
    }

    auto packed = types::readPackedType(data, 0);
    if (!packed)
        return std::nullopt;

    auto type = types::unpackMessageType(*packed);
    if (!type) {
        LOG_ERROR(std::format("Failed to unpack type from code {:#04x}", *packed));
        return std::nullopt;
    }

    // Copy payload (excluding the header) into a new vector
    network::ByteBuffer payload(data.begin() + types::kPackedTypeSize, data.end());
    return ParsedRawMessage{*type, std::move(payload)};
}

}  // namespace dungeons::common::wire
