#pragma once

#include <common/strong_id.h>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "network/types.h"
#include "domain/types.h"
#include "network/session_fwd.h"

namespace dungeons::server::app {

class SessionRegistry {
public:
    bool addSession(std::shared_ptr<network::Session> session);

    bool removeSessionBySessionId(network::SessionId sid);

    [[nodiscard]] size_t size() const;

    [[nodiscard]] std::shared_ptr<network::Session> findSessionBySessionId(network::SessionId sid) const;

    [[nodiscard]] std::shared_ptr<network::Session> findSessionByPlayerId(domain::PlayerId pid) const;

    [[nodiscard]] bool isPlayerIdBound(domain::PlayerId pid) const;

    [[nodiscard]] bool isSessionIdBound(network::SessionId sid) const;

    [[nodiscard]] std::optional<domain::PlayerId> getPlayerIdBySessionId(network::SessionId sid) const;

    [[nodiscard]] std::optional<network::SessionId> getSessionIdByPlayerId(domain::PlayerId pid) const;

    [[nodiscard]] std::vector<std::shared_ptr<network::Session>> getAllSessions() const;

    bool bindPlayerToSession(domain::PlayerId pid, network::SessionId sid);

private:

    std::unordered_map<network::SessionId, std::shared_ptr<network::Session>, network::SessionHash> sessions_;
    std::unordered_map<domain::PlayerId, network::SessionId, domain::PlayerHash> player_to_session_;
    std::unordered_map<network::SessionId, domain::PlayerId, network::SessionHash> session_to_player_;
    mutable std::mutex mutex_;

    std::shared_ptr<network::Session> findSessionBySID(network::SessionId sid) const;
};

}  // namespace dungeons::server::app
