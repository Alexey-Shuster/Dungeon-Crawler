#pragma once

#include <chrono>

#include "event_base.h"

namespace dungeons::server::core {

// Создает: GameLoop
// Получает: GameManager
struct GameTickEvent final : Event {
    const std::chrono::steady_clock::time_point timestamp;

    explicit GameTickEvent(std::chrono::steady_clock::time_point ts)
        : timestamp(ts) {}

    EventType getType() const override {
        return EventType::Tick;
    }
};

}  // namespace dungeons::server::core
