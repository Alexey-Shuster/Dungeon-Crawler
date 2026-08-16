#pragma once

#include <boost/signals2.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <server/core/event_bus.h>
#include <server/network/events.h>
#include <unordered_map>
#include <vector>

#include "events.h"
#include "session_registry.h"

namespace dungeons::server::app {

class ConnectionManager : public std::enable_shared_from_this<ConnectionManager> {
public:
    [[nodiscard]] static std::shared_ptr<ConnectionManager> Create(std::shared_ptr<core::EventBus> event_bus,
                                                                   std::shared_ptr<SessionRegistry> session_registry);

private:
    explicit ConnectionManager(std::shared_ptr<core::EventBus> event_bus,
                               std::shared_ptr<SessionRegistry> session_registry)
        : event_bus_(std::move(event_bus))
        , session_registry_(std::move(session_registry)) {}

    void Initialize();

    void HandleClientConnectedEvent(const network::ClientConnectedEvent& event);
    void HandleClientDisconnectedEvent(const network::ClientDisconnectedEvent& event);
    void HandlePlayerReconnectRequestedEvent(const ReconnectRequestedEvent& event);
    void HandlePlayerAuthRequestedEvent(const AuthRequestedEvent& event);

private:
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<SessionRegistry> session_registry_;
    std::unordered_map<domain::PlayerId, std::chrono::steady_clock::time_point, domain::PlayerHash>
        disconnected_players_;
    std::vector<boost::signals2::scoped_connection> connections_;
    mutable std::mutex mtx_;
};
}  // namespace dungeons::server::app
