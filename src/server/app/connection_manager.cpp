#include "connection_manager.h"

#include <common/utility/logger.h>
#include <format>
#include <server/network/session.h>

namespace dungeons::server::app {

// TODO (DRUsmanov): Add a alias for long type names like std::shared_ptr<network::SessionRegistry>

constexpr std::chrono::seconds kPlayerReconnectTimeout{30};

std::shared_ptr<ConnectionManager> ConnectionManager::Create(std::shared_ptr<core::EventBus> event_bus,
                                                             std::shared_ptr<SessionRegistry> session_registry) {
    auto connection_manager =
        std::shared_ptr<ConnectionManager>(new ConnectionManager{std::move(event_bus), std::move(session_registry)});
    std::once_flag init_flag;
    std::call_once(init_flag, &ConnectionManager::Initialize, connection_manager.get());
    return connection_manager;
}

void ConnectionManager::Initialize() {
    connections_.emplace_back(event_bus_->subscribe<network::ClientConnectedEvent>(
        [weak_self = weak_from_this()](const network::ClientConnectedEvent& event) {
            if (auto shared_self = weak_self.lock()) {
                shared_self->HandleClientConnectedEvent(event);
            }
        }));
    connections_.emplace_back(event_bus_->subscribe<network::ClientDisconnectedEvent>(
        [weak_self = weak_from_this()](const network::ClientDisconnectedEvent& event) {
            if (auto shared_self = weak_self.lock()) {
                shared_self->HandleClientDisconnectedEvent(event);
            }
        }));
    connections_.emplace_back(event_bus_->subscribe<ReconnectRequestedEvent>(
        [weak_self = weak_from_this()](const ReconnectRequestedEvent& event) {
            if (auto shared_self = weak_self.lock()) {
                shared_self->HandlePlayerReconnectRequestedEvent(event);
            }
        }));
    connections_.emplace_back(
        event_bus_->subscribe<AuthRequestedEvent>([weak_self = weak_from_this()](const AuthRequestedEvent& event) {
            if (auto shared_self = weak_self.lock()) {
                shared_self->HandlePlayerAuthRequestedEvent(event);
            }
        }));
}

void ConnectionManager::HandleClientConnectedEvent(const network::ClientConnectedEvent& event) {
    auto session = event.session;
    if (!session) {
        return;
    }

    if (session_registry_->addSession(session)) {
        LOG_INFO(std::format("Client connected with session id: {}", session->getSessionId().value));
    } else {
        LOG_INFO(std::format("Failed to connect session with id: {}", session->getSessionId().value));
    }
}

void ConnectionManager::HandleClientDisconnectedEvent(const network::ClientDisconnectedEvent& event) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto session = event.session;
    if (!session) {
        return;
    }

    auto session_id = session->getSessionId();
    auto player_id = session_registry_->getPlayerIdBySessionId(session_id);
    if (player_id) {
        disconnected_players_[*player_id] = std::chrono::steady_clock::now();
    }

    if (session_registry_->removeSessionBySessionId(session_id)) {
        LOG_INFO(std::format("Client disconnected with session id: {}", session_id.value));
    } else {
        LOG_INFO(std::format("Failed to disconnect session with id: {}", session_id.value));
    }
}

void ConnectionManager::HandlePlayerReconnectRequestedEvent(const ReconnectRequestedEvent& event) {
    std::lock_guard<std::mutex> lock{mtx_};
    auto player_id = event.player_id;
    auto session_id = event.session_id;
    if (auto it = disconnected_players_.find(player_id); it != disconnected_players_.end()) {
        auto duration = std::chrono::steady_clock::now() - it->second;

        if (duration > kPlayerReconnectTimeout) {
            event_bus_->publish(PlayerReconnectionFailedEvent{session_id, player_id});
            LOG_INFO(std::format("Player {} failed to reconnect to session {}", player_id.value, session_id.value));
        } else if (session_registry_->bindPlayerToSession(player_id, session_id)) {
            event_bus_->publish(PlayerReconnectedEvent{session_id, player_id});
            LOG_INFO(std::format("Player {} reconnected to session {}", player_id.value, session_id.value));
        } else {
            event_bus_->publish(PlayerReconnectionFailedEvent{session_id, player_id});
            LOG_INFO(std::format("Player {} failed to reconnect to session {}", player_id.value, session_id.value));
        }
    } else {
        event_bus_->publish(PlayerReconnectionFailedEvent{session_id, player_id});
        LOG_INFO(std::format("Player {} failed to reconnect to session {}", player_id.value, session_id.value));
    }
    disconnected_players_.erase(player_id);
}

void ConnectionManager::HandlePlayerAuthRequestedEvent(const AuthRequestedEvent& event) {
    auto player_id = event.player_id;
    auto session_id = event.session_id;

    if (session_registry_->bindPlayerToSession(player_id, session_id)) {
        event_bus_->publish(PlayerAuthenticatedEvent{session_id, player_id});
        LOG_INFO(std::format("Player {} authenticated with session {}", player_id.value, session_id.value));
    } else {
        event_bus_->publish(PlayerAuthenticationFailedEvent{session_id, player_id});
        LOG_INFO(std::format("Player {} failed to authenticate with session {}", player_id.value, session_id.value));
    }
}

}  // namespace dungeons::server::app
