#include "response_sender.h"

#include "../common/serialization_game_state.h"
#include "../domain/create_game_state.h"

namespace network {
std::shared_ptr<ResponseSender> ResponseSender::create(std::shared_ptr<events::EventBus> event_bus,
                                                       std::shared_ptr<SessionRegistry> session_registry) {
    struct EnableMakeShared : ResponseSender {
        EnableMakeShared(std::shared_ptr<events::EventBus> bus, std::shared_ptr<SessionRegistry> reg) :
            ResponseSender(std::move(bus), std::move(reg)) {}
    };

    auto sender = std::make_shared<EnableMakeShared>(std::move(event_bus), std::move(session_registry));
    sender->initialize();

    return sender;
}

ResponseSender::ResponseSender(std::shared_ptr<events::EventBus> event_bus,
                               std::shared_ptr<SessionRegistry> session_registry) :
    event_bus_(std::move(event_bus)), session_registry_(std::move(session_registry)) {}

void ResponseSender::initialize() {
    subscribeWeak<events::PlayerAuthenticatedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kWelcome);
    });

    subscribeWeak<events::PlayerAuthenticationFailedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kAuthFailed, event.player_id.value);
    });

    subscribeWeak<events::PlayerReconnectedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kReconnected);
    });

    subscribeWeak<events::PlayerReconnectionFailedEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::NetworkMessageType::kNotReconnected);
    });

    subscribeWeak<events::LobbyCreatedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPartyCreated, event.lobby_id.value);
    });

    subscribeWeak<events::LobbyCreationFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPartyNotCreated);
    });

    subscribeWeak<events::ListLobbiesResponseEvent>(&ResponseSender::onListLobbiesResponse);

    subscribeWeak<events::ListLobbiesFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kListPartiesNotCreated);
    });

    subscribeWeak<events::JoinLobbyResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerJoinedParty);
    });

    subscribeWeak<events::JoinLobbyFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerNotJoinedParty);
    });

    subscribeWeak<events::LeaveLobbyResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerLeavedParty);
    });
    subscribeWeak<events::LeaveLobbyFailedResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kPlayerNotLeavedParty);
    });

    subscribeWeak<events::StartGameResponseEvent>([](auto self, const auto& event) {
        self->sendResponse(event, message::AppMessageType::kStartGame);
    });

    subscribeWeak<events::GameStateUpdateEvent>(&ResponseSender::onGameStateUpdate);
}

void ResponseSender::onListLobbiesResponse(const events::ListLobbiesResponseEvent& event) {
    std::vector<uint64_t> args(event.lobby_ids.begin(), event.lobby_ids.end());
    sendResponse(event, message::AppMessageType::kListPartiesCreated, args);
}

void ResponseSender::onGameStateUpdate(const events::GameStateUpdateEvent& event) {
    LOG_INFO("ResponseSender::onGameStateUpdate");
    if (event.dungeon_state.players.empty()) {
        return;
    }
    auto game_state = serialization::createGameStateDTO(event.dungeon_state);
    auto game_state_msg = serialization::serializeGameState(game_state);
    if (game_state_msg.has_value()) {
        // TODO: send to players in dungeon
        for (auto session : session_registry_->getAllSessions()) {
            if (session) {
                session->send(std::move(game_state_msg->message_data));
            }
        }
    }
}

}  // namespace network
