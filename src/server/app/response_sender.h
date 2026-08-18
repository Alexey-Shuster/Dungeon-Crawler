#pragma once

#include <boost/signals2/connection.hpp>
#include <common/network/raw_message.h>
#include <common/utility/logger.h>
#include <common/wire/serder.h>
#include <functional>
#include <memory>
#include <server/core/event_bus.h>
#include <server/domain/core/events.h>
#include <server/network/session_fwd.h>
#include <vector>

#include "events.h"
#include "session_registry.h"

namespace dungeons::server::app {

class ResponseSender : public std::enable_shared_from_this<ResponseSender> {
public:
    static std::shared_ptr<ResponseSender> create(std::shared_ptr<core::EventBus> event_bus,
                                                  std::shared_ptr<SessionRegistry> session_registry);

    ~ResponseSender() = default;

protected:
    ResponseSender(std::shared_ptr<core::EventBus> event_bus, std::shared_ptr<SessionRegistry> session_registry);

private:
    void initialize();

    void onPlayerAuthenticated(const PlayerAuthenticatedEvent& event);
    void onPlayerReconnected(const PlayerReconnectedEvent& event);
    void onPlayerReconnectionFailed(const PlayerReconnectionFailedEvent& event);
    void onLobbyCreationFailed(const domain::LobbyCreationFailedResponseEvent& event);
    void onListLobbiesFailed(const domain::ListLobbiesFailedResponseEvent& event);
    void onListLobbiesResponse(const domain::ListLobbiesResponseEvent& event);
    void onJoinLobbyResponse(const domain::JoinLobbyResponseEvent& event);
    void onJoinLobbyFailed(const domain::JoinLobbyFailedResponseEvent& event);
    void onLeaveLobbyResponse(const domain::LeaveLobbyResponseEvent& event);
    void onLeaveLobbyFailed(const domain::LeaveLobbyFailedResponseEvent& event);
    void onStartGameResponse(const domain::StartGameResponseEvent& event);
    void onGameStateUpdate(const domain::GameStateUpdateEvent& event);

    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<SessionRegistry> session_registry_;
    std::vector<boost::signals2::scoped_connection> connections_;

    template <class Event, class Callable, std::enable_if_t<!std::is_member_function_pointer_v<Callable>, int> = 0>
    void subscribeWeak(Callable&& handler);

    template <class Event>
    void subscribeWeakMethod(void (ResponseSender::*handler)(const Event&));

    template <typename EventT>
    std::shared_ptr<network::Session> getSessionForEvent(const EventT& event);

    template <typename EventType, typename MsgType, typename... Args>
    void sendResponse(const EventType& event, MsgType msg_type, Args&&... args);

    template <typename EventType, typename MsgType>
    void sendResponse(const EventType& event, MsgType msg_type, const std::vector<uint64_t>& args);
};

template <class Event, class Callable, std::enable_if_t<!std::is_member_function_pointer_v<Callable>, int>>
void ResponseSender::subscribeWeak(Callable&& handler) {
    auto weak = weak_from_this();
    connections_.push_back(
        event_bus_->subscribe<Event>([weak, handler = std::forward<Callable>(handler)](const Event& e) {
            if (auto self = weak.lock()) {
                handler(self, e);
            }
        }));
}

template <class Event>
void ResponseSender::subscribeWeakMethod(void (ResponseSender::*handler)(const Event&)) {
    auto weak = weak_from_this();
    connections_.push_back(event_bus_->subscribe<Event>([weak, handler](const Event& e) {
        if (auto self = weak.lock()) {
            ((*self).*handler)(e);
        }
    }));
}

template <typename EventT>
std::shared_ptr<network::Session> ResponseSender::getSessionForEvent(const EventT& event) {
    if constexpr (requires { event.session_id; }) {
        auto session = session_registry_->findSessionBySessionId(event.session_id);
        if (!session) {
            LOG_ERROR(std::format("Session #{} not found", event.session_id.value));
            return nullptr;
        }
        return session;
    } else if constexpr (requires { event.player_id; }) {
        auto session = session_registry_->findSessionByPlayerId(event.player_id);
        if (!session) {
            LOG_ERROR(std::format("Session not found for player #{}", event.player_id.value));
            return nullptr;
        }
        return session;
    } else {
        // no known ID
        return nullptr;
    }
}

template <typename EventType, typename MsgType, typename... Args>
void ResponseSender::sendResponse(const EventType& event, MsgType msg_type, Args&&... args) {
    auto event_type = core::eventTypeToString(event.getType());
    auto session = getSessionForEvent(event);
    if (!session) {
        LOG_ERROR(std::format("No session found for event {}, dropping response", event_type));
        return;
    }

    auto opt_buf = common::wire::serializeMessage(msg_type, std::forward<Args>(args)...);
    if (opt_buf.has_value()) {
        LOG_INFO(std::format("Queued {}", event_type));
        session->send(common::network::RawMessage(std::move(*opt_buf)));
    } else {
        LOG_ERROR(std::format("Failed to serialize message for event {}", event_type));
    }
}

template <typename EventType, typename MsgType>
void ResponseSender::sendResponse(const EventType& event, MsgType msg_type, const std::vector<uint64_t>& args) {
    auto event_type = core::eventTypeToString(event.getType());
    auto session = getSessionForEvent(event);
    if (!session) {
        LOG_ERROR(std::format("No session found for event {}, dropping response", event_type));
        return;
    }

    auto opt_buf = common::wire::serializeMessage(msg_type, std::move(args));
    if (opt_buf.has_value()) {
        LOG_INFO(std::format("Queued {}", event_type));
        session->send(common::network::RawMessage(std::move(*opt_buf)));
    } else {
        LOG_ERROR(std::format("Failed to serialize message for event {}", event_type));
    }
}

}  // namespace dungeons::server::app
