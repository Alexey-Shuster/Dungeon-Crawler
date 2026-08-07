#pragma once

#include <../common/types.h>
#include <chrono>
#include <unordered_map>

namespace lobby {
struct LobbyPlayerInfo {
    bool is_ready{false};
    std::chrono::steady_clock::time_point joined_at;
};

class Lobby {
public:
    Lobby(LobbyId lid, PlayerId pid);
    [[nodiscard]] bool addPlayer(PlayerId playerId);
    [[nodiscard]] bool removePlayer(PlayerId playerId);
    [[nodiscard]] bool containsPlayer(PlayerId playerId) const;
    [[nodiscard]] bool setReady(PlayerId playerId, bool is_ready);
    [[nodiscard]] bool checkAllReady() const;
    [[nodiscard]] bool isFull() const;
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] LobbyId getId() const;
    [[nodiscard]] size_t getPlayerCount() const;
    [[nodiscard]] PlayerId getLeader() const;
    [[nodiscard]] std::vector<PlayerId> getAllPlayers() const;

private:
    LobbyId id_{LobbyId{0}};
    std::unordered_map<PlayerId, LobbyPlayerInfo, StrongIdHash<PlayerId>> players_{};
    PlayerId leader_id_{PlayerId{0}};

    void assignNewLeader();
};

}  // namespace lobby
