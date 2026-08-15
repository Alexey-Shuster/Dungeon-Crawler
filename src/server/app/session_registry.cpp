#include "session_registry.h"

#include <boost/format.hpp>
#include <common/utility/logger.h>
#include <domain/types.h>
#include <network/session.h>
#include <ranges>

namespace dungeons::server::app {

size_t SessionRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

bool SessionRegistry::addSession(std::shared_ptr<network::Session> session) {
    if (!session) {
        LOG_ERROR("Session does not exist.");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    network::SessionId sid = session->getSessionId();
    if (sessions_.contains(sid)) {
        LOG_ERROR(std::format("Session with id {} already exists.", sid.value));
        return false;
    }
    sessions_[sid] = std::move(session);
    return true;
}

bool SessionRegistry::removeSessionBySessionId(network::SessionId sid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.erase(sid) == 0) {
        LOG_ERROR(std::format("Session with id {} was not found, may have been already deleted.", sid.value));
        return false;
    }
    auto s_to_p_it = session_to_player_.find(sid);
    if (s_to_p_it != session_to_player_.end()) {
        domain::PlayerId pid = s_to_p_it->second;
        player_to_session_.erase(pid);
        session_to_player_.erase(s_to_p_it);
    }
    return true;
}

std::shared_ptr<network::Session> SessionRegistry::findSessionBySessionId(network::SessionId sid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findSessionBySID(sid);
}

std::shared_ptr<network::Session> SessionRegistry::findSessionByPlayerId(domain::PlayerId pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto p_to_s_it = player_to_session_.find(pid);
    if (p_to_s_it == player_to_session_.end()) {
        LOG_ERROR(std::format("Session for player {} not found.", pid.value));
        return nullptr;
    }
    return findSessionBySID(p_to_s_it->second);
}

bool SessionRegistry::isPlayerIdBound(domain::PlayerId pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return player_to_session_.contains(pid);
}

bool SessionRegistry::isSessionIdBound(network::SessionId sid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return session_to_player_.contains(sid);
}

std::optional<domain::PlayerId> SessionRegistry::getPlayerIdBySessionId(network::SessionId sid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = session_to_player_.find(sid); it != session_to_player_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<network::SessionId> SessionRegistry::getSessionIdByPlayerId(domain::PlayerId pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = player_to_session_.find(pid); it != player_to_session_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<network::Session>> SessionRegistry::getAllSessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<network::Session>> copy;
    copy.reserve(sessions_.size());
    for (const auto& session : sessions_ | std::views::values) {
        copy.push_back(session);
    }
    return copy;
}

bool SessionRegistry::bindPlayerToSession(domain::PlayerId pid, network::SessionId sid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!sessions_.contains(sid)) {
        LOG_ERROR(std::format("Cannot bind player {} to non-existent session {}.", pid.value, sid.value));
        return false;
    }
    player_to_session_.emplace(pid, sid);
    session_to_player_.emplace(sid, pid);
    return true;
}

std::shared_ptr<network::Session> SessionRegistry::findSessionBySID(network::SessionId sid) const {
    auto s_it = sessions_.find(sid);
    if (s_it == sessions_.end() || s_it->second == nullptr) {
        LOG_ERROR(std::format("Session with id {} not found.", sid.value));
        return nullptr;
    }
    return s_it->second;
}

}  // namespace dungeons::server::app
