#pragma once

#include "event_type.h"

namespace dungeons::server::core {

struct Event {
    virtual ~Event() = default;

    [[nodiscard]] virtual EventType getType() const = 0;
};

}  // namespace dungeons::server::core
