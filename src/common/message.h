#pragma once

#include <cstdint>
#include <vector>

namespace network {
using MessageData = std::vector<uint8_t>;

struct Message {
    MessageData message_data;

    explicit Message(MessageData incoming_data) : message_data{std::move(incoming_data)} {}
};
}  // namespace network
