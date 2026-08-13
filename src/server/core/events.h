#pragma once

#include <chrono>

#include "event_base.h"

namespace dungeons::server::core {

// Создает: GameLoop
// Получает: GameManager
struct GameTickEvent final : core::Event {
    const std::chrono::steady_clock::time_point timestamp;

    explicit GameTickEvent(std::chrono::steady_clock::time_point ts)
        : timestamp(ts) {}

    core::EventType getType() const override {
        return core::EventType::Tick;
    }
};

}  // namespace dungeons::server::core
