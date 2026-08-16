#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <memory>
#include <server/core/event_bus.h>
#include <vector>

#include "core/events.h"
#include "lobby_fwd.h"
#include "lobby_registry_fwd.h"

namespace dungeons::server::domain {

class LobbyManager : public std::enable_shared_from_this<LobbyManager> {
public:
    [[nodiscard]] static std::shared_ptr<LobbyManager> create(boost::asio::io_context& io_context,
                                                              std::shared_ptr<core::EventBus> eventBus,
                                                              std::shared_ptr<LobbyRegistry> lobbyRegistry);

    LobbyManager(const LobbyManager&) = delete;

    LobbyManager& operator=(const LobbyManager&) = delete;

    LobbyManager(LobbyManager&&) = delete;

    LobbyManager& operator=(LobbyManager&&) = delete;

    ~LobbyManager() = default;

protected:
    LobbyManager(boost::asio::io_context& io_context,
                 std::shared_ptr<core::EventBus> eventBus,
                 std::shared_ptr<LobbyRegistry> lobbyRegistry);

private:
    void Initialize();

    void onCreateLobbyRequest(const CreateLobbyRequestEvent& event);

    void onJoinLobbyRequest(const JoinLobbyRequestEvent& event);

    void onLeaveLobbyRequest(const LeaveLobbyRequestEvent& event);

    void onPlayerReadyRequest(const PlayerReadyRequestEvent& event);

    void onListLobbiesRequest(const ListLobbiesRequestEvent& event);

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<LobbyRegistry> lobby_registry_;
    std::vector<boost::signals2::scoped_connection> connections_;

private:
    enum class LobbyLookupError {
        Success,
        PlayerNotInLobby,
        StaleMapping
    };

    struct LobbyLookupResult {
        std::shared_ptr<Lobby> lobby;
        LobbyLookupError error;
    };

    // Helper: retrieves the lobby for a player, logs errors, and cleans stale mappings.
    // Returns nullptr if player is not in a lobby or the lobby is missing.
    LobbyLookupResult getLobbyForPlayer(PlayerId player_id, LobbyId& out_lobby_id);

    template <typename Event, typename Callable>
    void subscribeWeak(Callable&& handler);

    template <typename EventType, typename... Args>
    void publishEvent(Args&&... args) {
        event_bus_->publish(EventType{std::forward<Args>(args)...});
    }
};

template <typename Event, typename Callable>
void LobbyManager::subscribeWeak(Callable&& handler) {
    auto weak = weak_from_this();
    connections_.emplace_back(event_bus_->subscribe<Event>(
        [weak, handler = std::forward<Callable>(handler), strand = strand_](const Event& e) {
            // Event is a simple POD, thus copying
            boost::asio::post(strand, [weak, handler, e]() {
                if (auto self = weak.lock()) {
                    std::invoke(handler, self.get(), e);
                }
            });
        }));
}

}  // namespace dungeons::server::domain
