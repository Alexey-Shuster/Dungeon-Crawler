#include "lobby.h"

#include <algorithm>
#include <common/utility/config.h>
#include <ranges>

namespace dungeons::server::domain {

Lobby::Lobby(LobbyId lid, PlayerId pid)
    : id_(lid)
    , leader_id_(pid) {
    // Player is NOT added here – caller must call addPlayer explicitly.
}

bool Lobby::addPlayer(PlayerId playerId) {
    if (containsPlayer(playerId))
        return false;

    if (isFull()) {
        return false;
    }

    LobbyPlayerInfo info;
    info.joined_at = std::chrono::steady_clock::now();
    players_.emplace(playerId, info);

    return true;
}

bool Lobby::removePlayer(PlayerId playerId) {
    auto it = players_.find(playerId);
    if (it == players_.end())
        return false;

    players_.erase(it);

    if (leader_id_ == playerId && !isEmpty())
        assignNewLeader();

    return true;
}

bool Lobby::containsPlayer(PlayerId playerId) const {
    return players_.contains(playerId);
}

bool Lobby::setReady(PlayerId playerId, bool is_ready) {
    auto it = players_.find(playerId);
    if (it == players_.end())
        return false;

    it->second.is_ready = is_ready;

    return true;
}

bool Lobby::checkAllReady() const {
    return std::ranges::all_of(players_ | std::views::values, [](const auto& info) {
        return info.is_ready;
    });
}

bool Lobby::isFull() const {
    return players_.size() == common::utility::getSettings().gameplay.lobby_max_players;
}

bool Lobby::isEmpty() const {
    return players_.empty();
}

LobbyId Lobby::getId() const {
    return id_;
}

size_t Lobby::getPlayerCount() const {
    return players_.size();
}

PlayerId Lobby::getLeader() const {
    return leader_id_;
}

void Lobby::assignNewLeader() {
    if (isEmpty()) {
        return;
    }
    // Pick the first player in the map as the new leader.
    leader_id_ = players_.begin()->first;
}

std::vector<PlayerId> Lobby::getAllPlayers() const {
    std::vector<PlayerId> player_ids;
    player_ids.reserve(players_.size());
    for (const auto& player_id : players_ | std::views::keys) {
        player_ids.push_back(player_id);
    }
    return player_ids;
}

}  // namespace dungeons::server::domain
