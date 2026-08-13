#pragma once

#include "core/event_base.h"
#include "types.h"

namespace dungeons::server::domain {

struct PlayerEvent : virtual core::Event {
    PlayerId player_id;

    explicit PlayerEvent(PlayerId pid) noexcept
        : player_id(pid) {}
};

struct LobbyEvent : virtual core::Event {
    LobbyId lobby_id;

    explicit LobbyEvent(LobbyId lid) noexcept
        : lobby_id(lid) {}
};

struct GameEvent : virtual core::Event {
    GameId game_id;

    explicit GameEvent(GameId gid) noexcept
        : game_id(gid) {}
};

}  // namespace dungeons::server::domain
