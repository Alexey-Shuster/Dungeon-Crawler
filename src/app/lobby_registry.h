#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/hash.h"
#include "../common/types.h"
#include "../domain/lobby.h"

namespace lobby {
class LobbyRegistry {
public:
    LobbyRegistry() = default;

    ~LobbyRegistry() = default;

    LobbyRegistry(const LobbyRegistry&) = delete;

    LobbyRegistry& operator=(const LobbyRegistry&) = delete;

    bool addLobby(std::shared_ptr<Lobby> lobby);

    bool removeLobby(LobbyId lobby_id);

    std::shared_ptr<Lobby> findLobby(LobbyId lobby_id) const;

    size_t size() const;

    std::vector<std::shared_ptr<Lobby>> getAllLobbies() const;

    bool addPlayerToLobby(PlayerId player_id, LobbyId lobby_id);

    bool removePlayerFromLobby(PlayerId player_id);

    std::optional<LobbyId> getPlayerLobby(PlayerId player_id) const;

    bool isPlayerInLobby(PlayerId player_id) const;

    LobbyId generateNextLobbyId();

private:
    std::unordered_map<LobbyId, std::shared_ptr<Lobby>, hash::Int64Hasher> lobbies_{};
    std::unordered_map<PlayerId, LobbyId, StrongIdHash<PlayerId>> player_to_lobby_{};
    std::unordered_map<LobbyId, std::unordered_set<PlayerId, StrongIdHash<PlayerId>>, hash::Int64Hasher> lobby_players_;
    LobbyId next_id_{LobbyId{1}};
};
}  // namespace lobby
