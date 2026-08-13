#include "response_sender.h"

#include <common/message_types.h>
#include <common/serialization_game_state.h>

#include "domain/dungeon/create_game_state.h"
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
        self->sendResponse(event, message::NetworkMessageType::kWelcome);
    });

    subscribeWeak<PlayerAuthenticationFailedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kAuthFailed, event.player_id.value);
    });

    subscribeWeak<PlayerReconnectedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kReconnected);
    });

    subscribeWeak<PlayerReconnectionFailedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kNotReconnected);
    });

    subscribeWeak<domain::LobbyCreatedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPartyCreated, event.lobby_id.value);
    });

    subscribeWeak<domain::LobbyCreationFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPartyNotCreated);
    });

    subscribeWeak<domain::ListLobbiesResponseEvent>(&ResponseSender::onListLobbiesResponse);

    subscribeWeak<domain::ListLobbiesFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kListPartiesNotCreated);
    });

    subscribeWeak<domain::JoinLobbyResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerJoinedParty);
    });

    subscribeWeak<domain::JoinLobbyFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerNotJoinedParty);
    });

    subscribeWeak<domain::LeaveLobbyResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerLeavedParty);
    });
    subscribeWeak<domain::LeaveLobbyFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerNotLeavedParty);
    });

    subscribeWeak<domain::StartGameResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kStartGame);
    });

    subscribeWeak<domain::GameStateUpdateEvent>(&ResponseSender::onGameStateUpdate);
}

void ResponseSender::onListLobbiesResponse(const domain::ListLobbiesResponseEvent& event) {
    std::vector<uint64_t> args(event.lobby_ids.begin(), event.lobby_ids.end());
    sendResponse(event, message::AppMessageType::kListPartiesCreated, args);
}

void ResponseSender::onGameStateUpdate(const domain::GameStateUpdateEvent& event) {
    LOG_INFO("ResponseSender::onGameStateUpdate");
    if (event.dungeon_state.players.empty()) {
        return;
    }
    auto game_state = domain::createGameStateDTO(event.dungeon_state);
    auto game_state_msg = serialization::serializeGameState(game_state);
    if (game_state_msg.has_value()) {
        // TODO: send to players in dungeon
        for (const auto& session : session_registry_->getAllSessions()) {
            if (session) {
                session->send(std::move(*game_state_msg));
            }
        }
    }
}

}  // namespace dungeons::server::app
