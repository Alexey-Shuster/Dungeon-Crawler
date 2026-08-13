#pragma once

#include <common/byte_buffer.h>
#include <common/logger.h>
#include <common/message_utils.h>
#include <format>
#include <optional>
#include <vector>

#include "raw_message.h"

namespace dungeons::server::network {

[[nodiscard]] inline RawMessage makeMessage(const message::MessageTypeVariant& type,
                                            serialization::ByteBuffer payload) {
    auto packed = packMessageType(type);
    if (!packed) {
        LOG_ERROR("Failed to pack message type – returning empty");
        return RawMessage{std::vector<uint8_t>{}};
    }
    std::vector<uint8_t> data;
    data.reserve(message::kPackedTypeSize + payload.size());
    message::appendPackedType(*packed, data);
    data.insert(data.end(), payload.begin(), payload.end());  // simple copy
    return RawMessage{std::move(data)};
}

[[nodiscard]] inline std::optional<std::pair<message::MessageTypeVariant, serialization::ByteBuffer>> parseMessage(
    const RawMessage& msg) {
    const auto& data = msg.message_data;
    if (data.size() < message::kPackedTypeSize) {
        LOG_ERROR("Message too short to contain header");
        return std::nullopt;
    }

    auto packed = message::readPackedType(data, 0);
    if (!packed)
        return std::nullopt;

    auto type = message::unpackMessageType(*packed);
    if (!type) {
        LOG_ERROR(std::format("Failed to unpack type from code {:#04x}", *packed));
        return std::nullopt;
    }

    // Copy payload (excluding the header) into a new vector
    serialization::ByteBuffer payload(data.begin() + message::kPackedTypeSize, data.end());
    return std::pair{*type, std::move(payload)};
}

}  // namespace dungeons::server::network
