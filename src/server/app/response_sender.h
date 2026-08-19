#pragma once

#include <boost/signals2/connection.hpp>
#include <common/network/byte_buffer.h>
#include <common/wire/serder.h>
#include <memory>
#include <server/core/event_bus.h>
#include <server/domain/core/events.h>
#include <server/network/session_fwd.h>
#include <vector>

#include "events.h"
#include "session_registry_fwd.h"

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

    template <typename EventType, typename MsgType, typename... Args>
    void sendResponse(const EventType& event, MsgType msg_type, Args&&... args);

    template <typename EventT>
    std::shared_ptr<network::Session> getSessionForEvent(const EventT& event);

    std::shared_ptr<network::Session> findSessionBySessionId(network::SessionId id) const;
    std::shared_ptr<network::Session> findSessionByPlayerId(domain::PlayerId id) const;

    static void sendResponseImpl(const std::shared_ptr<network::Session>& session,
                                 std::string_view event_type_name,
                                 std::optional<common::network::ByteBuffer> opt_buf);
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

template <typename EventType, typename MsgType, typename... Args>
void ResponseSender::sendResponse(const EventType& event, MsgType msg_type, Args&&... args) {
    auto session = getSessionForEvent(event);
    auto opt_buf = common::wire::serializeMessage(msg_type, std::forward<Args>(args)...);
    sendResponseImpl(session, core::eventTypeToString(event.getType()), std::move(opt_buf));
}

template <typename EventT>
std::shared_ptr<network::Session> ResponseSender::getSessionForEvent(const EventT& event) {
    if constexpr (requires { event.session_id; }) {
        return findSessionBySessionId(event.session_id);
    } else if constexpr (requires { event.player_id; }) {
        return findSessionByPlayerId(event.player_id);
    } else {
        return nullptr;
    }
}

}  // namespace dungeons::server::app
