#pragma once

#include "byte_buffer.h"

namespace network {

struct [[nodiscard]] Message {
    ByteBuffer buffer;

    explicit Message(ByteBuffer&& buf)
        : buffer(std::move(buf)) {}

    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;
};

}  // namespace network
