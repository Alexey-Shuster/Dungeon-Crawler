#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <mutex>
#include <vector>

#include "../domain/game/dungeon_registry.h"
#include "../infra/eventbus.h"
#include "events.h"
#include "lobby_registry.h"

namespace game {

class GameManager : public std::enable_shared_from_this<GameManager> {
public:
    explicit GameManager(boost::asio::io_context& io_context,
                         std::shared_ptr<events::EventBus> event_bus,
                         std::shared_ptr<lobby::LobbyRegistry> lobby_registry);

    [[nodiscard]] static std::shared_ptr<GameManager> create(boost::asio::io_context& io_context,
                                                             std::shared_ptr<events::EventBus> event_bus,
                                                             std::shared_ptr<lobby::LobbyRegistry> lobby_registry);

private:
    template <typename Event, typename Callable>
    void subscribeWeak(Callable&& handler) {
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

    template <typename EventType, typename... Args>
    void postEvent(Args&&... args) {
        EventType ev{std::forward<Args>(args)...};

        boost::asio::post(strand_, [event_bus = this->event_bus_, ev = std::move(ev)]() {
            event_bus->publish(ev);
        });
    }

private:
    void Initialize();

    void onStartGameRequestEvent(const events::StartGameRequestEvent& event);
    void onGameTickEvent(const events::GameTickEvent& event);
    void onMoveRequestEvent(const events::MoveRequestEvent& event);
    void onAttackRequestEvent(const events::AtackRequestEvent& event);

    std::shared_ptr<events::EventBus> event_bus_;
    std::shared_ptr<lobby::LobbyRegistry> lobby_registry_;
    dungeon::DungeonRegistry dungeon_registry_;
    std::vector<boost::signals2::scoped_connection> connections_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

}  // namespace game
