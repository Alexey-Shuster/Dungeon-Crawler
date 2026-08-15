#pragma once

#include <common/types/direction.h>
#include <core/event_type.h>
#include <vector>

#include "domain/dungeon/dungeon_state.h"
#include "event_base.h"

namespace dungeons::server::domain {

// ======================================================================
// CREATE LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct CreateLobbyRequestEvent : PlayerEvent {
    explicit CreateLobbyRequestEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::CreateLobbyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LobbyCreatedResponseEvent : PlayerEvent, LobbyEvent {
    explicit LobbyCreatedResponseEvent(PlayerId pid, LobbyId lid) noexcept
        : PlayerEvent(pid)
        , LobbyEvent(lid) {}

    core::EventType getType() const override {
        return core::EventType::LobbyCreatedResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LobbyCreationFailedResponseEvent : PlayerEvent {
    explicit LobbyCreationFailedResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::LobbyCreationFailedResponse;
    }
};

// ======================================================================
// LIST LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct ListLobbiesRequestEvent : PlayerEvent {
    explicit ListLobbiesRequestEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::ListLobbiesRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct ListLobbiesResponseEvent : PlayerEvent {
    std::vector<LobbyId> lobby_ids;

    explicit ListLobbiesResponseEvent(PlayerId pid, std::vector<LobbyId>& lby_ids) noexcept
        : PlayerEvent(pid)
        , lobby_ids(std::move(lby_ids)) {}

    core::EventType getType() const override {
        return core::EventType::ListLobbiesResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct ListLobbiesFailedResponseEvent : PlayerEvent {
    explicit ListLobbiesFailedResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::ListLobbiesFailedResponse;
    }
};

// ======================================================================
// JOIN LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct JoinLobbyRequestEvent : PlayerEvent, LobbyEvent {
    explicit JoinLobbyRequestEvent(PlayerId pid, LobbyId lid) noexcept
        : PlayerEvent(pid)
        , LobbyEvent(lid) {}

    core::EventType getType() const override {
        return core::EventType::JoinLobbyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyResponseEvent : PlayerEvent {
    explicit JoinLobbyResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::JoinLobbyResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyFailedResponseEvent : PlayerEvent {
    explicit JoinLobbyFailedResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::JoinLobbyFailedResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyNotExistsResponseEvent : PlayerEvent {
    explicit JoinLobbyNotExistsResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::JoinLobbyNotExistsResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyFullResponseEvent : PlayerEvent {
    explicit JoinLobbyFullResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::JoinLobbyFullResponse;
    }
};

// ======================================================================
// LEAVE LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct LeaveLobbyRequestEvent : PlayerEvent {
    explicit LeaveLobbyRequestEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::LeaveLobbyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LeaveLobbyResponseEvent : PlayerEvent {
    explicit LeaveLobbyResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::LeaveLobbyResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LeaveLobbyFailedResponseEvent : PlayerEvent {
    explicit LeaveLobbyFailedResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::LeaveLobbyFailedResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LeaveLobbyNotConsistsInLobbyResponseEvent : PlayerEvent {
    explicit LeaveLobbyNotConsistsInLobbyResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::LeaveLobbyNotConsistsInLobbyResponse;
    }
};

// ======================================================================
// PLAYER READY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct PlayerReadyRequestEvent : PlayerEvent {
    bool is_ready;

    explicit PlayerReadyRequestEvent(PlayerId pid, bool ready) noexcept
        : PlayerEvent(pid)
        , is_ready(ready) {}
    core::EventType getType() const override {
        return core::EventType::PlayerReadyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct PlayerReadyResponseEvent : PlayerEvent {
    explicit PlayerReadyResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::PlayerReadyResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct PlayerReadyFailedResponseEvent : PlayerEvent {
    explicit PlayerReadyFailedResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::PlayerReadyFailedResponse;
    }
};

// ======================================================================
// START GAME
// ======================================================================

// Создает: MessageRouter
// Получает: GameManager
struct StartGameRequestEvent : PlayerEvent {
    explicit StartGameRequestEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::StartGameRequest;
    }
};

// Создает: GameManager
// Получает: ResponseSender
struct StartGameResponseEvent : PlayerEvent, GameEvent {
    explicit StartGameResponseEvent(PlayerId pid, GameId gid) noexcept
        : PlayerEvent(pid)
        , GameEvent(gid) {}

    core::EventType getType() const override {
        return core::EventType::StartGameResponse;
    }
};

// Создает: GameManager
// Получает: ResponseSender
struct StartGameLobbyNotReadyResponseEvent : PlayerEvent {
    explicit StartGameLobbyNotReadyResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::StartGameLobbyNotReadyResponse;
    }
};

// Создает: GameManager
// Получает: ResponseSender
struct StartGameNotTheLeaderResponseEvent : PlayerEvent {
    explicit StartGameNotTheLeaderResponseEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}
    core::EventType getType() const override {
        return core::EventType::StartGameNotTheLeaderResponse;
    }
};

// ======================================================================
// MOVE
// ======================================================================

// Создает: MessageRouter
// Получает: GameManager
struct MoveRequestEvent : PlayerEvent {
    common::types::Direction direction;

    explicit MoveRequestEvent(PlayerId pid, common::types::Direction dir) noexcept
        : PlayerEvent(pid)
        , direction(dir) {}

    core::EventType getType() const override {
        return core::EventType::MoveRequest;
    }
};

// ======================================================================
// ATTACK
// ======================================================================

// Создает: MessageRouter
// Получает: GameManager
struct AtackRequestEvent : PlayerEvent {
    explicit AtackRequestEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::AttackRequest;
    }
};

// ======================================================================
// STATE UPDATE
// ======================================================================

// Создает: GameManager
// Получает: ResponseSender
struct GameStateUpdateEvent : GameEvent {
    DungeonState dungeon_state;

    explicit GameStateUpdateEvent(GameId gid, DungeonState ds) noexcept
        : GameEvent(gid)
        , dungeon_state(std::move(ds)) {}

    core::EventType getType() const override {
        return core::EventType::GameStateUpdate;
    }
};

// ======================================================================
// GAME OVER
// ======================================================================

// Создает: GameManager
// Получает: ResponseSender
struct GameOverEvent : PlayerEvent {
    explicit GameOverEvent(PlayerId pid) noexcept
        : PlayerEvent(pid) {}
    core::EventType getType() const override {
        return core::EventType::GameOver;
    }
};

}  // namespace dungeons::server::domain
