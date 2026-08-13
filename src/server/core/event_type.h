#pragma once

#include <cstdint>  // IWYU pragma: keep
#include <string_view>

namespace dungeons::server::core {

enum class EventType : uint8_t {
    // Base / raw
    ClientConnected,
    ClientDisconnected,
    RawMessageReceived,
    Ping,
    Pong,

    // Authentication
    AuthRequested,
    PlayerAuthenticated,
    PlayerAuthenticationFailed,

    // Reconnection
    ReconnectRequested,
    PlayerReconnected,
    PlayerReconnectionFailed,

    // Lobby
    CreateLobbyRequest,
    LobbyCreatedResponse,
    LobbyCreationFailedResponse,

    ListLobbiesRequest,
    ListLobbiesResponse,
    ListLobbiesFailedResponse,

    JoinLobbyRequest,
    JoinLobbyResponse,
    JoinLobbyFailedResponse,
    JoinLobbyNotExistsResponse,
    JoinLobbyFullResponse,

    LeaveLobbyRequest,
    LeaveLobbyResponse,
    LeaveLobbyFailedResponse,
    LeaveLobbyNotConsistsInLobbyResponse,
    PlayerReadyRequest,
    PlayerReadyResponse,
    PlayerReadyFailedResponse,

    // Game
    StartGameRequest,
    StartGameResponse,
    StartGameLobbyNotReadyResponse,
    StartGameNotTheLeaderResponse,
    Tick,
    GameOver,

    // Domain
    MoveRequest,
    AttackRequest,

    GameStateUpdate,
    // ...
};

constexpr std::string_view eventTypeToString(EventType type) {
    switch (type) {
        case EventType::ClientConnected:
            return "ClientConnected";
        case EventType::ClientDisconnected:
            return "ClientDisconnected";
        case EventType::RawMessageReceived:
            return "RawMessageReceived";
        case EventType::Ping:
            return "Ping";
        case EventType::Pong:
            return "Pong";
        case EventType::AuthRequested:
            return "AuthRequested";
        case EventType::PlayerAuthenticated:
            return "PlayerAuthenticated";
        case EventType::PlayerAuthenticationFailed:
            return "PlayerAuthenticationFailed";
        case EventType::ReconnectRequested:
            return "ReconnectRequested";
        case EventType::PlayerReconnected:
            return "PlayerReconnected";
        case EventType::PlayerReconnectionFailed:
            return "PlayerReconnectionFailed";
        case EventType::CreateLobbyRequest:
            return "CreateLobbyRequest";
        case EventType::LobbyCreatedResponse:
            return "LobbyCreatedResponse";
        case EventType::LobbyCreationFailedResponse:
            return "LobbyCreationFailedResponse";
        case EventType::ListLobbiesRequest:
            return "ListLobbiesRequest";
        case EventType::ListLobbiesResponse:
            return "ListLobbiesResponse";
        case EventType::ListLobbiesFailedResponse:
            return "ListLobbiesFailedResponse";
        case EventType::JoinLobbyRequest:
            return "JoinLobbyRequest";
        case EventType::JoinLobbyResponse:
            return "JoinLobbyResponse";
        case EventType::JoinLobbyFailedResponse:
            return "JoinLobbyFailedResponse";
        case EventType::JoinLobbyNotExistsResponse:
            return "JoinLobbyNotExistsResponse";
        case EventType::JoinLobbyFullResponse:
            return "JoinLobbyFullResponse";
        case EventType::LeaveLobbyRequest:
            return "LeaveLobbyRequest";
        case EventType::LeaveLobbyResponse:
            return "LeaveLobbyResponse";
        case EventType::LeaveLobbyFailedResponse:
            return "LeaveLobbyFailedResponse";
        case EventType::LeaveLobbyNotConsistsInLobbyResponse:
            return "LeaveLobbyNotConsistsInLobbyResponse";
        case EventType::PlayerReadyRequest:
            return "PlayerReadyRequest";
        case EventType::PlayerReadyResponse:
            return "PlayerReadyResponse";
        case EventType::PlayerReadyFailedResponse:
            return "PlayerReadyFailedResponse";
        case EventType::StartGameRequest:
            return "StartGameRequest";
        case EventType::StartGameResponse:
            return "StartGameResponse";
        case EventType::StartGameLobbyNotReadyResponse:
            return "StartGameLobbyNotReadyResponse";
        case EventType::StartGameNotTheLeaderResponse:
            return "StartGameNotTheLeaderResponse";
        case EventType::Tick:
            return "Tick";
        case EventType::MoveRequest:
            return "MoveRequest";
        case EventType::AttackRequest:
            return "AttackRequest";
        case EventType::GameStateUpdate:
            return "GameStateUpdate";
        default:
            return "Unknown";
    }
}

}  // namespace dungeons::server::core
