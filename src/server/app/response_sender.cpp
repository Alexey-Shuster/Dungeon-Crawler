#include "response_sender.h"

#include <common/types/message_types.h>
#include <common/utility/logger.h>
#include <common/wire/serder_game_state.h>
#include <format>
#include <server/network/session.h>

#include "session_registry.h"

namespace dungeons::server::app {

std::shared_ptr<ResponseSender> ResponseSender::create(std::shared_ptr<core::EventBus> event_bus,
                                                       std::shared_ptr<SessionRegistry> session_registry) {
    struct EnableMakeShared : ResponseSender {
        EnableMakeShared(std::shared_ptr<core::EventBus> bus, std::shared_ptr<SessionRegistry> reg)
            : ResponseSender(std::move(bus), std::move(reg)) {}
    };

    auto sender = std::make_shared<EnableMakeShared>(std::move(event_bus), std::move(session_registry));
    sender->initialize();

    return sender;
}

ResponseSender::ResponseSender(std::shared_ptr<core::EventBus> event_bus,
                               std::shared_ptr<SessionRegistry> session_registry)
    : event_bus_(std::move(event_bus))
    , session_registry_(std::move(session_registry)) {}

void ResponseSender::initialize() {
    subscribeWeakMethod<PlayerAuthenticatedEvent>(&ResponseSender::onPlayerAuthenticated);
    subscribeWeakMethod<PlayerReconnectedEvent>(&ResponseSender::onPlayerReconnected);
    subscribeWeakMethod<PlayerReconnectionFailedEvent>(&ResponseSender::onPlayerReconnectionFailed);

    subscribeWeak<PlayerAuthenticationFailedEvent>([](const auto& self, const auto& event) {
        self->sendResponse(event, dc_NetMsg::kAuthFailed, event.player_id.value);
    });
    subscribeWeak<domain::LobbyCreatedResponseEvent>([](const auto& self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPartyCreated, event.lobby_id.value);
    });

    subscribeWeakMethod<domain::LobbyCreationFailedResponseEvent>(&ResponseSender::onLobbyCreationFailed);
    subscribeWeakMethod<domain::ListLobbiesFailedResponseEvent>(&ResponseSender::onListLobbiesFailed);
    subscribeWeakMethod<domain::ListLobbiesResponseEvent>(&ResponseSender::onListLobbiesResponse);
    subscribeWeakMethod<domain::JoinLobbyResponseEvent>(&ResponseSender::onJoinLobbyResponse);
    subscribeWeakMethod<domain::JoinLobbyFailedResponseEvent>(&ResponseSender::onJoinLobbyFailed);
    subscribeWeakMethod<domain::LeaveLobbyResponseEvent>(&ResponseSender::onLeaveLobbyResponse);
    subscribeWeakMethod<domain::LeaveLobbyFailedResponseEvent>(&ResponseSender::onLeaveLobbyFailed);
    subscribeWeakMethod<domain::StartGameResponseEvent>(&ResponseSender::onStartGameResponse);

    subscribeWeakMethod<domain::GameStateUpdateEvent>(&ResponseSender::onGameStateUpdate);
}

#pragma region onMethods
void ResponseSender::onPlayerAuthenticated(const PlayerAuthenticatedEvent& event) {
    sendResponse(event, dc_NetMsg::kWelcome);
}
void ResponseSender::onPlayerReconnected(const PlayerReconnectedEvent& event) {
    sendResponse(event, dc_NetMsg::kReconnected);
}

void ResponseSender::onPlayerReconnectionFailed(const PlayerReconnectionFailedEvent& event) {
    sendResponse(event, dc_NetMsg::kNotReconnected);
}

void ResponseSender::onLobbyCreationFailed(const domain::LobbyCreationFailedResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kPartyNotCreated);
}

void ResponseSender::onListLobbiesFailed(const domain::ListLobbiesFailedResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kListPartiesNotCreated);
}

void ResponseSender::onJoinLobbyResponse(const domain::JoinLobbyResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kPlayerJoinedParty);
}

void ResponseSender::onJoinLobbyFailed(const domain::JoinLobbyFailedResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kPlayerNotJoinedParty);
}

void ResponseSender::onLeaveLobbyResponse(const domain::LeaveLobbyResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kPlayerLeavedParty);
}

void ResponseSender::onLeaveLobbyFailed(const domain::LeaveLobbyFailedResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kPlayerNotLeavedParty);
}

void ResponseSender::onStartGameResponse(const domain::StartGameResponseEvent& event) {
    sendResponse(event, dc_AppMsg::kStartGame);
}

void ResponseSender::onListLobbiesResponse(const domain::ListLobbiesResponseEvent& event) {
    std::vector<uint64_t> args(event.lobby_ids.begin(), event.lobby_ids.end());
    sendResponse(event, common::types::AppMessageType::kListPartiesCreated, args);
}
#pragma endregion

void ResponseSender::onGameStateUpdate(const domain::GameStateUpdateEvent& event) {
    LOG_INFO("ResponseSender::onGameStateUpdate");
    // TODO: refine logic
    if (!event.dungeon_snapshot || event.dungeon_snapshot->players.empty()) {
        return;
    }

    auto game_state_msg = common::wire::serializeGameState(*event.dungeon_snapshot);
    if (game_state_msg.has_value()) {
        // TODO: send to players in dungeon
        for (const auto& session : session_registry_->getAllSessions()) {
            if (session) {
                session->send(common::network::RawMessage{std::move(*game_state_msg)});
            }
        }
    }
}

std::shared_ptr<network::Session> ResponseSender::findSessionBySessionId(network::SessionId id) {
    auto session = session_registry_->findSessionBySessionId(id);
    if (!session) {
        LOG_ERROR(std::format("Session #{} not found", id.get()));
    }
    return session;
}

std::shared_ptr<network::Session> ResponseSender::findSessionByPlayerId(domain::PlayerId id) {
    auto session = session_registry_->findSessionByPlayerId(id);
    if (!session) {
        LOG_ERROR(std::format("Session not found for player #{}", id.get()));
    }
    return session;
}

void ResponseSender::sendResponseImpl(const std::shared_ptr<network::Session>& session,
                                      std::string_view event_type_name,
                                      std::optional<common::network::ByteBuffer> opt_buf) {
    if (!session) {
        LOG_ERROR(std::format("No session found for event {}, dropping response", event_type_name));
        return;
    }

    if (opt_buf.has_value()) {
        LOG_INFO(std::format("Queued {}", event_type_name));
        session->send(common::network::RawMessage(std::move(*opt_buf)));
    } else {
        LOG_ERROR(std::format("Failed to serialize message for event {}", event_type_name));
    }
}

}  // namespace dungeons::server::app
