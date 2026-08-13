#pragma once

#include "domain/event_base.h"
#include "network/events.h"

namespace dungeons::server::app {

// ======================================================================
// PLAYER AUTHENTICATION
// ======================================================================

// Создает: MessageRouter
// Получает: ConnectionManager
struct AuthRequestedEvent : network::SessionEvent, domain::PlayerEvent {
    explicit AuthRequestedEvent(network::SessionId sid, domain::PlayerId pid) noexcept
        : network::SessionEvent(sid)
        , PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::AuthRequested;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerAuthenticatedEvent : network::SessionEvent, domain::PlayerEvent {
    explicit PlayerAuthenticatedEvent(network::SessionId sid, domain::PlayerId pid) noexcept
        : network::SessionEvent(sid)
        , PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::PlayerAuthenticated;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerAuthenticationFailedEvent : network::SessionEvent, domain::PlayerEvent {
    explicit PlayerAuthenticationFailedEvent(network::SessionId sid, domain::PlayerId pid) noexcept
        : network::SessionEvent(sid)
        , PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::PlayerAuthenticationFailed;
    }
};

// ======================================================================
// PLAYER RECONNECTION
// ======================================================================

// Создает: MessageRouter
// Получает: ConnectionManager
struct ReconnectRequestedEvent : network::SessionEvent, domain::PlayerEvent {
    explicit ReconnectRequestedEvent(network::SessionId sid, domain::PlayerId pid) noexcept
        : network::SessionEvent(sid)
        , PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::ReconnectRequested;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerReconnectedEvent : network::SessionEvent, domain::PlayerEvent {
    explicit PlayerReconnectedEvent(network::SessionId sid, domain::PlayerId pid) noexcept
        : network::SessionEvent(sid)
        , PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::PlayerReconnected;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PlayerReconnectionFailedEvent : network::SessionEvent, domain::PlayerEvent {
    explicit PlayerReconnectionFailedEvent(network::SessionId sid, domain::PlayerId pid) noexcept
        : network::SessionEvent(sid)
        , PlayerEvent(pid) {}

    core::EventType getType() const override {
        return core::EventType::PlayerReconnectionFailed;
    }
};

}
