#include "session_registry.h"

#include <boost/format.hpp>

#include "logger.h"
#include "session.h"

namespace network {

size_t SessionRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

bool SessionRegistry::addSession(std::shared_ptr<Session> session) {
    if (!session) {
        LOG_ERROR("Session does not exist.");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    SessionId sid = session->getSessionId();
    if (sessions_.contains(sid)) {
        LOG_ERROR(std::format("Session with id {} already exists.", sid.value));
        return false;
    }
    sessions_[sid] = std::move(session);
    return true;
}

bool SessionRegistry::removeSessionBySessionId(SessionId sid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.erase(sid) == 0) {
        LOG_ERROR(std::format("Session with id {} was not found, may have been already deleted.", sid.value));
        return false;
    }
    auto s_to_p_it = session_to_player_.find(sid);
    if (s_to_p_it != session_to_player_.end()) {
        PlayerId pid = s_to_p_it->second;
        player_to_session_.erase(pid);
        session_to_player_.erase(s_to_p_it);
    }
    return true;
}

std::shared_ptr<Session> SessionRegistry::findSessionBySessionId(SessionId sid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findSessionBySID(sid);
}

std::shared_ptr<Session> SessionRegistry::findSessionByPlayerId(PlayerId pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto p_to_s_it = player_to_session_.find(pid);
    if (p_to_s_it == player_to_session_.end()) {
        LOG_ERROR(std::format("Session for player {} not found.", pid.value));
        return nullptr;
    }
    return findSessionBySID(p_to_s_it->second);
}

bool SessionRegistry::isPlayerIdBound(PlayerId pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return player_to_session_.find(pid) != player_to_session_.end();
}

bool SessionRegistry::isSessionIdBound(SessionId sid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return session_to_player_.find(sid) != session_to_player_.end();
}

std::optional<PlayerId> SessionRegistry::getPlayerIdBySessionId(SessionId sid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = session_to_player_.find(sid); it != session_to_player_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<SessionId> SessionRegistry::getSessionIdByPlayerId(PlayerId pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = player_to_session_.find(pid); it != player_to_session_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<Session>> SessionRegistry::getAllSessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Session>> copy;
    copy.reserve(sessions_.size());
    for (const auto& [id, session] : sessions_) {
        copy.push_back(session);
    }
    return copy;
}

bool SessionRegistry::bindPlayerToSession(PlayerId pid, SessionId sid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!sessions_.contains(sid)) {
        LOG_ERROR(std::format("Cannot bind player {} to non-existent session {}.", pid.value, sid.value));
        return false;
    }
    player_to_session_.emplace(pid, sid);
    session_to_player_.emplace(sid, pid);
    return true;
}

std::shared_ptr<Session> SessionRegistry::findSessionBySID(SessionId sid) const {
    auto s_it = sessions_.find(sid);
    if (s_it == sessions_.end() || s_it->second == nullptr) {
        LOG_ERROR(std::format("Session with id {} not found.", sid.value));
        return nullptr;
    }
    return s_it->second;
}

}  // namespace network
