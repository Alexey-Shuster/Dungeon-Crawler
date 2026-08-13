#pragma once

#include <memory>

#include "core/event_base.h"
#include "core/event_type.h"
#include "raw_message.h"
#include "session_event.h"
#include "session_fwd.h"

namespace dungeons::server::network {

struct ClientEvent : core::Event {
    std::shared_ptr<Session> session;

    explicit ClientEvent(std::shared_ptr<Session> s = nullptr) noexcept
        : session(std::move(s)) {}
};

struct ClientConnectedEvent : ClientEvent {
    using ClientEvent::ClientEvent;

    core::EventType getType() const override {
        return core::EventType::ClientConnected;
    }
};

struct ClientDisconnectedEvent : ClientEvent {
    using ClientEvent::ClientEvent;

    core::EventType getType() const override {
        return core::EventType::ClientDisconnected;
    }
};

// Создает: MessageRouter
// Получает: ConnectionManager
struct PingEvent : SessionEvent {
    explicit PingEvent(SessionId sid) noexcept
        : SessionEvent(sid) {}

    core::EventType getType() const override {
        return core::EventType::Ping;
    }
};

// Создает: ConnectionManager
// Получает: ResponseSender
struct PongEvent : SessionEvent {
    explicit PongEvent(SessionId sid) noexcept
        : SessionEvent(sid) {}

    core::EventType getType() const override {
        return core::EventType::Pong;
    }
};

// Создает: Session
// Получает: MessageRouter
struct RawMessageReceivedEvent : public SessionEvent {
    RawMessage message;

    RawMessageReceivedEvent(SessionId sid, RawMessage msg) noexcept
        : SessionEvent(sid)
        , message(std::move(msg)) {}

    core::EventType getType() const override {
        return core::EventType::RawMessageReceived;
    }
};

}  // namespace dungeons::server::network
