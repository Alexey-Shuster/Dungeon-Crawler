#include "lobby_manager.h"

#include <common/utility/logger.h>
#include <format>

#include "lobby.h"
#include "lobby_registry.h"

namespace dungeons::server::domain {

std::shared_ptr<LobbyManager> LobbyManager::create(boost::asio::io_context& io_context,
                                                   std::shared_ptr<core::EventBus> eventBus,
                                                   std::shared_ptr<LobbyRegistry> lobbyRegistry) {
    struct EnableMakeShared : LobbyManager {
        EnableMakeShared(boost::asio::io_context& ioc,
                         std::shared_ptr<core::EventBus> eb,
                         std::shared_ptr<LobbyRegistry> lr)
            : LobbyManager(ioc, std::move(eb), std::move(lr)) {}
    };
    auto manager = std::make_shared<EnableMakeShared>(io_context, std::move(eventBus), std::move(lobbyRegistry));

    manager->Initialize();

    return manager;
}

LobbyManager::LobbyManager(boost::asio::io_context& io_context,
                           std::shared_ptr<core::EventBus> eventBus,
                           std::shared_ptr<LobbyRegistry> lobbyRegistry)
    : strand_(boost::asio::make_strand(io_context))
    , event_bus_(std::move(eventBus))
    , lobby_registry_(std::move(lobbyRegistry)) {}

void LobbyManager::Initialize() {
    subscribeWeak<CreateLobbyRequestEvent>(&LobbyManager::onCreateLobbyRequest);

    subscribeWeak<JoinLobbyRequestEvent>(&LobbyManager::onJoinLobbyRequest);

    subscribeWeak<LeaveLobbyRequestEvent>(&LobbyManager::onLeaveLobbyRequest);

    subscribeWeak<PlayerReadyRequestEvent>(&LobbyManager::onPlayerReadyRequest);

    subscribeWeak<ListLobbiesRequestEvent>(&LobbyManager::onListLobbiesRequest);
}

void LobbyManager::onCreateLobbyRequest(const CreateLobbyRequestEvent& event) {
    // Quick check – registry will also check, but this avoids creating a lobby if unnecessary
    if (lobby_registry_->isPlayerInLobby(event.player_id)) {
        LOG_INFO(std::format("Player {} already in a lobby", event.player_id.value));
        publishEvent<LobbyCreationFailedResponseEvent>(event.player_id);
        return;
    }

    auto newId = lobby_registry_->generateNextLobbyId();
    auto lobby = std::make_shared<Lobby>(newId, event.player_id);  // does NOT add player

    if (!lobby_registry_->addLobby(lobby)) {
        LOG_ERROR(std::format("Failed to add lobby {} to registry", newId.value));
        publishEvent<LobbyCreationFailedResponseEvent>(event.player_id);
        return;
    }

    if (!lobby_registry_->addPlayerToLobby(event.player_id, newId)) {
        LOG_ERROR(
            std::format("Failed to add player {} to lobby {}; removing lobby", event.player_id.value, newId.value));
        lobby_registry_->removeLobby(newId);
        publishEvent<LobbyCreationFailedResponseEvent>(event.player_id);
        return;
    }

    publishEvent<LobbyCreatedResponseEvent>(event.player_id, newId);
}

void LobbyManager::onJoinLobbyRequest(const JoinLobbyRequestEvent& event) {
    auto lobby = lobby_registry_->findLobby(event.lobby_id);
    if (!lobby) {
        publishEvent<JoinLobbyNotExistsResponseEvent>(event.player_id);
        return;
    }
    if (lobby->isFull()) {
        publishEvent<JoinLobbyFullResponseEvent>(event.player_id);
        return;
    }

    auto success = lobby_registry_->addPlayerToLobby(event.player_id, event.lobby_id);
    if (success) {
        publishEvent<JoinLobbyResponseEvent>(event.player_id);
    } else {
        LOG_INFO(
            std::format("Join request failed for player {} to lobby {}", event.player_id.value, event.lobby_id.value));
        publishEvent<JoinLobbyFailedResponseEvent>(event.player_id);
    }
}

void LobbyManager::onLeaveLobbyRequest(const LeaveLobbyRequestEvent& event) {
    LobbyId lobbyId{0};
    auto result = getLobbyForPlayer(event.player_id, lobbyId);

    if (result.error == LobbyLookupError::PlayerNotInLobby) {
        publishEvent<LeaveLobbyNotConsistsInLobbyResponseEvent>(event.player_id);
        return;
    }
    if (result.error == LobbyLookupError::StaleMapping) {
        publishEvent<LeaveLobbyFailedResponseEvent>(event.player_id);
        return;
    }

    bool removed = lobby_registry_->removePlayerFromLobby(event.player_id);
    if (removed) {
        publishEvent<LeaveLobbyResponseEvent>(event.player_id);
    } else {
        LOG_INFO(std::format("Leave lobby request failed for player {}", event.player_id.value));
        publishEvent<LeaveLobbyFailedResponseEvent>(event.player_id);
    }
}

void LobbyManager::onPlayerReadyRequest(const PlayerReadyRequestEvent& event) {
    LobbyId lobbyId{0};
    auto result = getLobbyForPlayer(event.player_id, lobbyId);

    // TODO: should we know anything about response events at this level ?
    if (result.error == LobbyLookupError::PlayerNotInLobby || result.error == LobbyLookupError::StaleMapping) {
        publishEvent<PlayerReadyFailedResponseEvent>(event.player_id);
        return;
    }

    if (result.lobby->setReady(event.player_id, event.is_ready)) {
        publishEvent<PlayerReadyResponseEvent>(event.player_id);
    } else {
        LOG_ERROR(std::format("Failed to set ready for player {} in lobby {}", event.player_id.value, lobbyId.value));
        publishEvent<PlayerReadyFailedResponseEvent>(event.player_id);
    }
}

void LobbyManager::onListLobbiesRequest(const ListLobbiesRequestEvent& event) {
    auto lobbies = lobby_registry_->getAllLobbies();
    std::vector<LobbyId> lobbies_ids{};
    for (auto& lobby : lobbies) {
        lobbies_ids.emplace_back(lobby->getId());
    }
    LOG_INFO(std::format("LobbyId list created with elements count #{}", lobbies_ids.size()));
    publishEvent<ListLobbiesResponseEvent>(event.player_id, lobbies_ids);
}

LobbyManager::LobbyLookupResult LobbyManager::getLobbyForPlayer(PlayerId player_id, LobbyId& out_lobby_id) {
    auto optLobbyId = lobby_registry_->getPlayerLobby(player_id);
    if (!optLobbyId) {
        LOG_INFO(std::format("Player {} not in any lobby", player_id.value));
        return {nullptr, LobbyLookupError::PlayerNotInLobby};
    }

    out_lobby_id = *optLobbyId;
    auto lobby = lobby_registry_->findLobby(out_lobby_id);
    if (!lobby) {
        LOG_ERROR(std::format("Lobby {} missing for player {} (stale mapping)", out_lobby_id.value, player_id.value));
        // Clean up the stale mapping
        lobby_registry_->removePlayerFromLobby(player_id);
        return {nullptr, LobbyLookupError::StaleMapping};
    }

    return {lobby, LobbyLookupError::Success};
}

}  // namespace dungeons::server::domain
