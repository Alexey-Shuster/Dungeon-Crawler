#pragma once

#include "core/event_base.h"
#include "types.h"

namespace dungeons::server::network {

struct SessionEvent : virtual core::Event {
    SessionId session_id;

    explicit SessionEvent(SessionId sid) noexcept
        : session_id(sid) {}
};

}  // namespace dungeons::server::network
