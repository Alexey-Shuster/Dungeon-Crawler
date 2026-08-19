#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <server/core/event_bus.h>
#include <server/core/events.h>
#include <server/domain/core/events.h>
#include <server/domain/dungeon/dungeon_registry_fwd.h>
#include <server/domain/lobby/lobby_registry_fwd.h>
#include <vector>

namespace dungeons::server::app {

class GameManager : public std::enable_shared_from_this<GameManager> {
public:
    explicit GameManager(boost::asio::io_context& io_context,
                         std::shared_ptr<core::EventBus> event_bus,
                         std::shared_ptr<domain::LobbyRegistry> lobby_registry);

    [[nodiscard]] static std::shared_ptr<GameManager> create(boost::asio::io_context& io_context,
                                                             std::shared_ptr<core::EventBus> event_bus,
                                                             std::shared_ptr<domain::LobbyRegistry> lobby_registry);

private:
    template <typename Event, typename Callable>
    void subscribeWeak(Callable&& handler);

    template <typename EventType, typename... Args>
    void postEvent(Args&&... args) {
        EventType ev{std::forward<Args>(args)...};

        boost::asio::post(strand_, [event_bus = this->event_bus_, ev = std::move(ev)]() {
            event_bus->publish(ev);
        });
    }

private:
    void Initialize();

    void onStartGameRequestEvent(const domain::StartGameRequestEvent& event);
    void onGameTickEvent(const core::GameTickEvent& event);
    void onMoveRequestEvent(const domain::MoveRequestEvent& event) const;
    void onAttackRequestEvent(const domain::AtackRequestEvent& event) const;

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<domain::LobbyRegistry> lobby_registry_;
    std::shared_ptr<domain::DungeonRegistry> dungeon_registry_ = std::make_shared<domain::DungeonRegistry>();
    std::vector<boost::signals2::scoped_connection> connections_;
};

template <typename Event, typename Callable>
void GameManager::subscribeWeak(Callable&& handler) {
    auto weak = weak_from_this();
    connections_.emplace_back(
        event_bus_->subscribe<Event>([weak, handler = std::forward<Callable>(handler), this](const Event& e) {
            boost::asio::post(strand_, [weak, handler, e]() {
                if (auto self = weak.lock()) {
                    std::invoke(handler, self.get(), e);
                }
            });
        }));
}

}  // namespace dungeons::server::app
