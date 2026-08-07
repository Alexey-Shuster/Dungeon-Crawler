#include "lobby.h"

#include <algorithm>
#include <ranges>

#include "../common/config.h"

namespace {
const auto& cfg = config::get_settings();
}

namespace lobby {
Lobby::Lobby(LobbyId lid, PlayerId pid) : id_(lid), leader_id_(pid) {
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
    for (const auto& info : players_ | std::views::values) {
        if (!info.is_ready)
            return false;
    }

    return true;
}

bool Lobby::isFull() const {
    return players_.size() == cfg.gameplay.lobby_max_players;
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
    for (const auto& [player_id, _] : players_) {
        player_ids.push_back(player_id);
    }
    return player_ids;
}

}  // namespace lobby
