#include "lobby_registry.h"

#include <format>
#include <ranges>

#include "../common/logger.h"
#include "../domain/lobby.h"

namespace lobby {
bool LobbyRegistry::addLobby(std::shared_ptr<Lobby> lobby) {
    if (!lobby) {
        LOG_ERROR("Attempt to add null lobby to registry");
        return false;
    }

    LobbyId id = lobby->getId();

    auto [it, inserted] = lobbies_.emplace(id, std::move(lobby));
    if (!inserted) {
        LOG_ERROR(std::format("Lobby with id {} already exists.", static_cast<uint64_t>(id)));
        return false;
    }

    LOG_INFO(std::format("Lobby {} added to registry.", id.value));

    return true;
}

bool LobbyRegistry::removeLobby(LobbyId lobby_id) {
    auto it = lobbies_.find(lobby_id);
    if (it == lobbies_.end()) {
        LOG_ERROR(std::format("Attempt to remove non‑existent lobby {}", lobby_id.value));
        return false;
    }

    auto playersIt = lobby_players_.find(lobby_id);
    if (playersIt != lobby_players_.end()) {
        for (PlayerId pid : playersIt->second) {
            player_to_lobby_.erase(pid);
        }
        lobby_players_.erase(playersIt);
    }

    lobbies_.erase(it);
    LOG_INFO(std::format("Lobby {} removed from registry.", lobby_id.value));

    return true;
}

std::shared_ptr<Lobby> LobbyRegistry::findLobby(LobbyId lobby_id) const {
    auto it = lobbies_.find(lobby_id);
    if (it == lobbies_.end()) {
        return nullptr;
    }

    return it->second;
}

size_t LobbyRegistry::size() const {
    return lobbies_.size();
}

std::vector<std::shared_ptr<Lobby>> LobbyRegistry::getAllLobbies() const {
    std::vector<std::shared_ptr<Lobby>> result;
    result.reserve(lobbies_.size());

    for (const auto& lobbyPtr : lobbies_ | std::views::values) {
        result.push_back(lobbyPtr);
    }

    return result;
}

bool LobbyRegistry::addPlayerToLobby(PlayerId player_id, LobbyId lobby_id) {
    // 1. Player already in lobby?
    if (isPlayerInLobby(player_id)) {
        LOG_INFO(std::format("Player {} already in lobby {}", player_id.value, player_to_lobby_.at(player_id).value));
        return false;
    }

    // 2. Lobby exists?
    auto lobbyIt = lobbies_.find(lobby_id);
    if (lobbyIt == lobbies_.end()) {
        LOG_ERROR(std::format("Lobby {} not found.", lobby_id.value));
        return false;
    }
    auto lobby_found = lobbyIt->second;

    // 3. Try to add player to the lobby
    if (!lobby_found->addPlayer(player_id)) {
        LOG_INFO(std::format("Failed to add player {} to lobby {}.", player_id.value, lobby_found->getId().value));
        return false;
    }

    auto& playerSet = lobby_players_[lobby_id];
    auto [setIt, setInserted] = playerSet.insert(player_id);

    // 4. Update mapping
    auto [mapIt, mapInserted] = player_to_lobby_.emplace(player_id, lobby_id);
    if (!setInserted || !mapInserted) {
        if (setInserted)
            playerSet.erase(player_id);
        if (mapInserted)
            player_to_lobby_.erase(mapIt);
        LOG_ERROR(std::format("Failed to insert mapping for player {}; rolled back.", player_id.value));
        if (!lobby_found->removePlayer(player_id)) {
            LOG_ERROR(std::format("Failed to remove player {} from lobby {} while rolling back.",
                                  player_id.value,
                                  lobby_id.value));
        }
        return false;
    }

    LOG_INFO(std::format("Player {} added to lobby {}. Mapping set.", player_id.value, lobby_id.value));

    return true;
}

bool LobbyRegistry::removePlayerFromLobby(PlayerId player_id) {
    // 1. Find the lobby via mapping
    auto mapIt = player_to_lobby_.find(player_id);
    if (mapIt == player_to_lobby_.end()) {
        LOG_INFO(std::format("Player {} not in any lobby. Remove skipped.", player_id.value));
        return false;
    }
    auto lobbyId = mapIt->second;

    // 2. Get lobby
    auto lobbyIt = lobbies_.find(lobbyId);
    if (lobbyIt == lobbies_.end()) {
        // Inconsistent state: mapping exists but lobby missing
        player_to_lobby_.erase(mapIt);
        LOG_ERROR(
            std::format("Lobby {} missing for player {}; removed stale mapping.", lobbyId.value, player_id.value));
        return false;
    }
    auto lobby = lobbyIt->second;

    // 3. Remove player from lobby
    if (!lobby->removePlayer(player_id)) {
        // Stale mapping: remove to allow recovery
        player_to_lobby_.erase(mapIt);
        LOG_ERROR(std::format("Failed to remove player {} from lobby {}.", player_id.value, lobbyId.value));
        return false;
    }

    // 4. Erase mapping
    auto& playerSet = lobby_players_[lobbyId];
    playerSet.erase(player_id);
    player_to_lobby_.erase(mapIt);

    LOG_INFO(std::format("Player {} removed from lobby {}.", player_id.value, lobbyId.value));

    // 5. If lobby is empty, remove it entirely
    if (lobby->isEmpty()) {
        lobbies_.erase(lobbyIt);
        lobby_players_.erase(lobbyId);
        LOG_INFO(std::format("Lobby {} empty - deleted.", lobbyId.value));
    }

    return true;
}

std::optional<LobbyId> LobbyRegistry::getPlayerLobby(PlayerId player_id) const {
    auto it = player_to_lobby_.find(player_id);
    if (it == player_to_lobby_.end())
        return std::nullopt;

    return it->second;
}

bool LobbyRegistry::isPlayerInLobby(PlayerId player_id) const {
    return player_to_lobby_.contains(player_id);
}

LobbyId LobbyRegistry::generateNextLobbyId() {
    return next_id_++;
}
}  // namespace lobby
