#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include <optional>

#include "types.h"

namespace network {
    class Session;

    class SessionRegistry {
    public:
        bool addSession(std::shared_ptr<Session> session);

        bool removeSessionBySessionId(SessionId sid);

        [[nodiscard]] size_t size() const;

        [[nodiscard]] std::shared_ptr<Session> findSessionBySessionId(SessionId sid) const;

        [[nodiscard]] std::shared_ptr<Session> findSessionByPlayerId(PlayerId pid) const;

        [[nodiscard]] bool isPlayerIdBound(PlayerId pid) const;

        [[nodiscard]] bool isSessionIdBound(SessionId sid) const;

        [[nodiscard]] std::optional<PlayerId> getPlayerIdBySessionId(SessionId sid) const;

        [[nodiscard]] std::optional<SessionId> getSessionIdByPlayerId(PlayerId pid) const;

        [[nodiscard]] std::vector<std::shared_ptr<Session> > getAllSessions() const;

        bool bindPlayerToSession(PlayerId pid, SessionId sid);

    private:
        std::unordered_map<SessionId, std::shared_ptr<Session>, StrongIdHash<SessionId> > sessions_;
        std::unordered_map<PlayerId, SessionId, StrongIdHash<PlayerId> > player_to_session_;
        std::unordered_map<SessionId, PlayerId, StrongIdHash<SessionId> > session_to_player_;
        mutable std::mutex mutex_;

        std::shared_ptr<Session> findSessionBySID(SessionId sid) const;
    };
} // namespace network
