#pragma once

#include <boost/signals2/connection.hpp>
#include <memory>
#include <string>
#include <vector>

#include "eventbus.h"
#include "events.h"
#include "logger.h"
#include "message_types.h"
#include "serialization.h"
#include "session.h"
#include "session_registry.h"

namespace network {
class ResponseSender : public std::enable_shared_from_this<ResponseSender> {
public:
    static std::shared_ptr<ResponseSender> create(std::shared_ptr<events::EventBus> event_bus,
                                                  std::shared_ptr<SessionRegistry> session_registry);

    ~ResponseSender() = default;

protected:
    ResponseSender(std::shared_ptr<events::EventBus> event_bus, std::shared_ptr<SessionRegistry> session_registry);

private:
    void initialize();

    void onListLobbiesResponse(const events::ListLobbiesResponseEvent& event);
    void onGameStateUpdate(const events::GameStateUpdateEvent& event);

    std::shared_ptr<events::EventBus> event_bus_;
    std::shared_ptr<SessionRegistry> session_registry_;
    std::vector<boost::signals2::scoped_connection> connections_;

    // lambdas & callable objects
    template <class Event,
              class Callable,
              typename std::enable_if_t<!std::is_member_function_pointer_v<Callable>, int> = 0>
    void subscribeWeak(Callable&& handler) {
        auto weak = weak_from_this();
        connections_.push_back(
            event_bus_->subscribe<Event>([weak, handler = std::forward<Callable>(handler)](const Event& e) {
                if (auto self = weak.lock()) {
                    handler(self, e);
                }
            }));
    }

    // pointer-to-member-function
    template <class Event,
              class Handler,
              typename std::enable_if_t<std::is_member_function_pointer_v<Handler>, int> = 0>
    void subscribeWeak(Handler handler) {
        auto weak = weak_from_this();
        connections_.push_back(event_bus_->subscribe<Event>([weak, handler](const Event& e) {
            if (auto self = weak.lock()) {
                ((*self).*handler)(e);
            }
        }));
    }

    template <typename EventT>
    std::shared_ptr<Session> getSessionForEvent(const EventT& event) {
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
    void sendResponse(const EventType& event, MsgType msg_type, Args&&... args) {
        auto event_type = events::eventTypeToString(event.getType());
        auto session = getSessionForEvent(event);
        if (!session) {
            LOG_ERROR(std::format("No session found for event {}, dropping response", event_type));
            return;
        }

        auto opt_msg = serialization::serializeMessage(msg_type, std::forward<Args>(args)...);
        if (opt_msg.has_value()) {
            LOG_INFO(std::format("Queued {}", event_type));
            session->send(MessageData(std::move(opt_msg.value().message_data)));
        } else {
            LOG_ERROR(std::format("Failed to serialize message for event {}", event_type));
            return;
        }
    }

    template <typename EventType, typename MsgType>
    void sendResponse(const EventType& event, MsgType msg_type, const std::vector<uint64_t>& args) {
        auto event_type = events::eventTypeToString(event.getType());
        auto session = getSessionForEvent(event);
        if (!session) {
            LOG_ERROR(std::format("No session found for event {}, dropping response", event_type));
            return;
        }

        auto opt_msg = serialization::serializeMessage(msg_type, args);
        if (opt_msg.has_value()) {
            LOG_INFO(std::format("Queued {}", event_type));
            session->send(MessageData(std::move(opt_msg.value().message_data)));
        } else {
            LOG_ERROR(std::format("Failed to serialize message for event {}", event_type));
        }
    }
};
}  // namespace network
