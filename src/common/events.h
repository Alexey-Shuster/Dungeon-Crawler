#pragma once

#include <chrono>
#include <memory>

#include "../domain/dungeon/dungeon.h"
#include "direction.h"
#include "event_type.h"
#include "message.h"
#include "types.h"

namespace network {

class Session;

}  // namespace network

namespace events {
struct Event {
    virtual ~Event() = default;

    virtual EventType getType() const = 0;
};

struct ClientEvent : public Event {
    std::shared_ptr<network::Session> session;

    explicit ClientEvent(std::shared_ptr<network::Session> s = nullptr) noexcept : session(std::move(s)) {}
};

struct ClientConnectedEvent : public ClientEvent {
    using ClientEvent::ClientEvent;

    EventType getType() const override {
        return EventType::ClientConnected;
    }
};

struct ClientDisconnectedEvent : public ClientEvent {
    using ClientEvent::ClientEvent;

    EventType getType() const override {
        return EventType::ClientDisconnected;
    }
};

struct PlayerEvent : virtual public Event {
    PlayerId player_id;

    explicit PlayerEvent(PlayerId pid) noexcept : player_id(pid) {}
};

struct SessionEvent : virtual public Event {
    SessionId session_id;

    explicit SessionEvent(SessionId sid) noexcept : session_id(sid) {}
};

struct LobbyEvent : virtual public Event {
    LobbyId lobby_id;

    explicit LobbyEvent(LobbyId lid) noexcept : lobby_id(lid) {}
};

struct GameEvent : virtual public Event {
    GameId game_id;

    explicit GameEvent(GameId gid) noexcept : game_id(gid) {}
};

// ======================================================================
// PING
// ======================================================================

// Создает: MessageRouter
// Получает: ??? TODO (DRUsmanov) : think about it
struct PingEvent : public SessionEvent {
    explicit PingEvent(SessionId sid) noexcept : SessionEvent(sid) {}

    EventType getType() const override {
        return EventType::Ping;
    }
};

// Создает: ResponseSender
// Получает: ??? TODO (DRUsmanov) : think about it
struct PongEvent : public SessionEvent {
    explicit PongEvent(SessionId sid) noexcept : SessionEvent(sid) {}

    EventType getType() const override {
        return EventType::Pong;
    }
};

// ======================================================================
// RAW MESSAGE
// ======================================================================

// Создает: Session
// Получает: MessageRouter
struct RawMessageReceivedEvent : public SessionEvent {
    network::Message message;

    RawMessageReceivedEvent(SessionId sid, network::Message msg) noexcept :
        SessionEvent(sid), message(std::move(msg)) {}

    EventType getType() const override {
        return EventType::RawMessageReceived;
    }
};

// ======================================================================
// PLAYER AUTHENTICATION
// ======================================================================

// Создает: MessageRouter
// Получает: ConnectionManager
struct AuthRequestedEvent : public SessionEvent, public PlayerEvent {
    explicit AuthRequestedEvent(SessionId sid, PlayerId pid) noexcept : SessionEvent(sid), PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::AuthRequested;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerAuthenticatedEvent : public SessionEvent, public PlayerEvent {
    explicit PlayerAuthenticatedEvent(SessionId sid, PlayerId pid) noexcept : SessionEvent(sid), PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::PlayerAuthenticated;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerAuthenticationFailedEvent : public SessionEvent, public PlayerEvent {
    explicit PlayerAuthenticationFailedEvent(SessionId sid, PlayerId pid) noexcept :
        SessionEvent(sid), PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::PlayerAuthenticationFailed;
    }
};

// ======================================================================
// PLAYER RECONNECTION
// ======================================================================

// Создает: MessageRouter
// Получает: ConnectionManager
struct ReconnectRequestedEvent : public SessionEvent, public PlayerEvent {
    explicit ReconnectRequestedEvent(SessionId sid, PlayerId pid) noexcept : SessionEvent(sid), PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::ReconnectRequested;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerReconnectedEvent : public SessionEvent, public PlayerEvent {
    explicit PlayerReconnectedEvent(SessionId sid, PlayerId pid) noexcept : SessionEvent(sid), PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::PlayerReconnected;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerReconnectionFailedEvent : public SessionEvent, public PlayerEvent {
    explicit PlayerReconnectionFailedEvent(SessionId sid, PlayerId pid) noexcept :
        SessionEvent(sid), PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::PlayerReconnectionFailed;
    }
};

// ======================================================================
// CREATE LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct CreateLobbyRequestEvent : public PlayerEvent {
    explicit CreateLobbyRequestEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::CreateLobbyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LobbyCreatedResponseEvent : public PlayerEvent, public LobbyEvent {
    explicit LobbyCreatedResponseEvent(PlayerId pid, LobbyId lid) noexcept : PlayerEvent(pid), LobbyEvent(lid) {}

    EventType getType() const override {
        return EventType::LobbyCreatedResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LobbyCreationFailedResponseEvent : public PlayerEvent {
    explicit LobbyCreationFailedResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::LobbyCreationFailedResponse;
    }
};

// ======================================================================
// LIST LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct ListLobbiesRequestEvent : public PlayerEvent {
    explicit ListLobbiesRequestEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::ListLobbiesRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct ListLobbiesResponseEvent : public PlayerEvent {
    std::vector<LobbyId> lobby_ids;

    explicit ListLobbiesResponseEvent(PlayerId pid, std::vector<LobbyId>& lbyids) noexcept :
        PlayerEvent(pid), lobby_ids(std::move(lbyids)) {}

    EventType getType() const override {
        return EventType::ListLobbiesResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct ListLobbiesFailedResponseEvent : public PlayerEvent {
    explicit ListLobbiesFailedResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::ListLobbiesFailedResponse;
    }
};

// ======================================================================
// JOIN LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct JoinLobbyRequestEvent : public PlayerEvent, public LobbyEvent {
    explicit JoinLobbyRequestEvent(PlayerId pid, LobbyId lid) noexcept : PlayerEvent(pid), LobbyEvent(lid) {}

    EventType getType() const override {
        return EventType::JoinLobbyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyResponseEvent : public PlayerEvent {
    explicit JoinLobbyResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::JoinLobbyResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyFailedResponseEvent : public PlayerEvent {
    explicit JoinLobbyFailedResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::JoinLobbyFailedResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyNotExistsResponseEvent : public PlayerEvent {
    explicit JoinLobbyNotExistsResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::JoinLobbyNotExistsResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct JoinLobbyFullResponseEvent : public PlayerEvent {
    explicit JoinLobbyFullResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::JoinLobbyFullResponse;
    }
};

// ======================================================================
// LEAVE LOBBY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct LeaveLobbyRequestEvent : public PlayerEvent {
    explicit LeaveLobbyRequestEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::LeaveLobbyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LeaveLobbyResponseEvent : public PlayerEvent {
    explicit LeaveLobbyResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::LeaveLobbyResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LeaveLobbyFailedResponseEvent : public PlayerEvent {
    explicit LeaveLobbyFailedResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::LeaveLobbyFailedResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct LeaveLobbyNotConsistsInLobbyResponseEvent : public PlayerEvent {
    explicit LeaveLobbyNotConsistsInLobbyResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::LeaveLobbyNotConsistsInLobbyResponse;
    }
};

// ======================================================================
// PLAYER READY
// ======================================================================

// Создает: MessageRouter
// Получает: LobbyManager
struct PlayerReadyRequestEvent : public PlayerEvent {
    bool is_ready;

    explicit PlayerReadyRequestEvent(PlayerId pid, bool ready) noexcept : PlayerEvent(pid), is_ready(ready) {}
    EventType getType() const override {
        return EventType::PlayerReadyRequest;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct PlayerReadyResponseEvent : public PlayerEvent {
    explicit PlayerReadyResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::PlayerReadyResponse;
    }
};

// Создает: LobbyManager
// Получает: ResponseSender
struct PlayerReadyFailedResponseEvent : public PlayerEvent {
    explicit PlayerReadyFailedResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::PlayerReadyFailedResponse;
    }
};

// ======================================================================
// START GAME
// ======================================================================

// Создает: MessageRouter
// Получает: GameManager
struct StartGameRequestEvent : public PlayerEvent {
    explicit StartGameRequestEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::StartGameRequest;
    }
};

// Создает: GameManager
// Получает: ResponseSender
struct StartGameResponseEvent : public PlayerEvent, public GameEvent {
    explicit StartGameResponseEvent(PlayerId pid, GameId gid) noexcept : PlayerEvent(pid), GameEvent(gid) {}

    EventType getType() const override {
        return EventType::StartGameResponse;
    }
};

// Создает: GameManager
// Получает: ResponseSender
struct StartGameLobbyNotReadyResponseEvent : public PlayerEvent {
    explicit StartGameLobbyNotReadyResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::StartGameLobbyNotReadyResponse;
    }
};

// Создает: GameManager
// Получает: ResponseSender
struct StartGameNotTheLeaderResponseEvent : public PlayerEvent {
    explicit StartGameNotTheLeaderResponseEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}
    EventType getType() const override {
        return EventType::StartGameNotTheLeaderResponse;
    }
};

// ======================================================================
// TICK
// ======================================================================

// Создает: GameLoop
// Получает: GameManager
struct GameTickEvent final : public Event {
    const std::chrono::steady_clock::time_point timestamp;

    explicit GameTickEvent(std::chrono::steady_clock::time_point ts) : timestamp(ts) {}

    EventType getType() const override {
        return EventType::Tick;
    }
};

// ======================================================================
// MOVE
// ======================================================================

// Создает: MessageRouter
// Получает: GameManager
struct MoveRequestEvent : public PlayerEvent {
    Direction direction;

    explicit MoveRequestEvent(PlayerId pid, Direction dir) noexcept : PlayerEvent(pid), direction(dir) {}

    EventType getType() const override {
        return EventType::MoveRequest;
    }
};

// ======================================================================
// ATTACK
// ======================================================================

// Создает: MessageRouter
// Получает: GameManager
struct AtackRequestEvent : public PlayerEvent {
    explicit AtackRequestEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}

    EventType getType() const override {
        return EventType::AtackRequest;
    }
};

// ======================================================================
// STATE UPDATE
// ======================================================================

// Создает: GameManager
// Получает: ResponseSender
struct GameStateUpdateEvent : public GameEvent {
    dungeon::DungeonState dungeon_state;

    explicit GameStateUpdateEvent(GameId gid, dungeon::DungeonState ds) noexcept :
        GameEvent(gid), dungeon_state(std::move(ds)) {}

    EventType getType() const override {
        return EventType::GameStateUpdate;
    }
};

// ======================================================================
// GAME OVER
// ======================================================================

// Создает: GameManager
// Получает: ResponseSender
struct GameOverEvent : public PlayerEvent {
    explicit GameOverEvent(PlayerId pid) noexcept : PlayerEvent(pid) {}
    EventType getType() const override {
        return EventType::GameOver;
    }
};

}  // namespace events
