#pragma once

#include <common/byte_buffer.h>

namespace dungeons::server::network {

using RawMessageData = serialization::ByteBuffer;

struct RawMessage {
    RawMessageData message_data;

    explicit RawMessage(RawMessageData incoming_data)
        : message_data{std::move(incoming_data)} {}
};

}  // namespace dungeons::server::network
