#include "response_sender.h"

#include <common/types/message_types.h>
#include <common/wire/serialization_game_state.h>
#include <domain/dungeon/create_game_state.h>

#include "events.h"

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
    subscribeWeak<PlayerAuthenticatedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::NetworkMessageType::kWelcome);
    });

    subscribeWeak<PlayerAuthenticationFailedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::NetworkMessageType::kAuthFailed, event.player_id.value);
    });

    subscribeWeak<PlayerReconnectedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::NetworkMessageType::kReconnected);
    });

    subscribeWeak<PlayerReconnectionFailedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::NetworkMessageType::kNotReconnected);
    });

    subscribeWeak<domain::LobbyCreatedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPartyCreated, event.lobby_id.value);
    });

    subscribeWeak<domain::LobbyCreationFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPartyNotCreated);
    });

    subscribeWeak<domain::ListLobbiesResponseEvent>(&ResponseSender::onListLobbiesResponse);

    subscribeWeak<domain::ListLobbiesFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kListPartiesNotCreated);
    });

    subscribeWeak<domain::JoinLobbyResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPlayerJoinedParty);
    });

    subscribeWeak<domain::JoinLobbyFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPlayerNotJoinedParty);
    });

    subscribeWeak<domain::LeaveLobbyResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPlayerLeavedParty);
    });
    subscribeWeak<domain::LeaveLobbyFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kPlayerNotLeavedParty);
    });

    subscribeWeak<domain::StartGameResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, common::types::AppMessageType::kStartGame);
    });

    subscribeWeak<domain::GameStateUpdateEvent>(&ResponseSender::onGameStateUpdate);
}

void ResponseSender::onListLobbiesResponse(const domain::ListLobbiesResponseEvent& event) {
    std::vector<uint64_t> args(event.lobby_ids.begin(), event.lobby_ids.end());
    sendResponse(event, common::types::AppMessageType::kListPartiesCreated, args);
}

void ResponseSender::onGameStateUpdate(const domain::GameStateUpdateEvent& event) {
    LOG_INFO("ResponseSender::onGameStateUpdate");
    if (event.dungeon_state.players.empty()) {
        return;
    }
    auto game_state = domain::createGameStateDTO(event.dungeon_state);
    auto game_state_msg = common::wire::serializeGameState(game_state);
    if (game_state_msg.has_value()) {
        // TODO: send to players in dungeon
        for (const auto& session : session_registry_->getAllSessions()) {
            if (session) {
                session->send(common::network::RawMessage{std::move(*game_state_msg)});
            }
        }
    }
}

}  // namespace dungeons::server::app
