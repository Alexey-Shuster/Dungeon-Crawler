#pragma once
#include <boost/signals2.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "../common/events.h"
#include "../common/types.h"
#include "../infra/eventbus.h"
#include "session_registry.h"

namespace connection_manager {
class ConnectionManager : public std::enable_shared_from_this<ConnectionManager> {
public:
    [[nodiscard]] static std::shared_ptr<ConnectionManager> Create(
        std::shared_ptr<events::EventBus> event_bus,
        std::shared_ptr<network::SessionRegistry> session_registry);

private:
    explicit ConnectionManager(std::shared_ptr<events::EventBus> event_bus,
                               std::shared_ptr<network::SessionRegistry> session_registry) :
        event_bus_(std::move(event_bus)), session_registry_(std::move(session_registry)) {}

    void Initialize();
    void HandleClientConnectedEvent(const events::ClientConnectedEvent& event);
    void HandleClientDisconnectedEvent(const events::ClientDisconnectedEvent& event);
    void HandlePlayerReconnectRequestedEvent(const events::ReconnectRequestedEvent& event);
    void HandlePlayerAuthRequestedEvent(const events::AuthRequestedEvent& event);

private:
    std::shared_ptr<events::EventBus> event_bus_;
    std::shared_ptr<network::SessionRegistry> session_registry_;
    std::unordered_map<PlayerId, std::chrono::steady_clock::time_point, StrongIdHash<PlayerId>> disconnected_players_;
    std::vector<boost::signals2::scoped_connection> connections_;
    mutable std::mutex mtx_;
};
}  // namespace connection_manager
