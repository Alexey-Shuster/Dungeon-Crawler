#pragma once

#include <cstdint>
#include <vector>

namespace dungeons::common::wire {

/// @brief Тип для хранения аргументов сообщения
using MessageArgs = std::vector<uint64_t>;

}  // namespace dungeons::common::wire
