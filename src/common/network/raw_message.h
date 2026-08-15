#pragma once

#include "byte_buffer.h"

namespace dungeons::common::network {

struct [[nodiscard]] RawMessage {
    ByteBuffer buffer;

    explicit RawMessage(ByteBuffer&& buf)
        : buffer(std::move(buf)) {}

    RawMessage(const RawMessage&) = delete;
    RawMessage& operator=(const RawMessage&) = delete;

    RawMessage(RawMessage&&) noexcept = default;
    RawMessage& operator=(RawMessage&&) noexcept = default;
};

}  // namespace dungeons::common::network
