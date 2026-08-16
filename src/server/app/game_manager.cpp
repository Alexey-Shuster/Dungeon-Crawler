#include "game_manager.h"

#include <common/utility/config.h>
#include <common/utility/logger.h>
#include <format>
#include <server/domain/dungeon/dungeon.h>
#include <server/domain/lobby/lobby.h>
#include <server/domain/lobby/lobby_registry.h>

namespace dungeons::server::app {

GameManager::GameManager(boost::asio::io_context& io_context,
                         std::shared_ptr<core::EventBus> event_bus,
                         std::shared_ptr<domain::LobbyRegistry> lobby_registry)
    : strand_(io_context.get_executor())
    , event_bus_(std::move(event_bus))
    , lobby_registry_(std::move(lobby_registry)) {}

std::shared_ptr<GameManager> GameManager::create(boost::asio::io_context& io_context,
                                                 std::shared_ptr<core::EventBus> event_bus,
                                                 std::shared_ptr<domain::LobbyRegistry> lobby_registry) {
    auto manager = std::make_shared<GameManager>(io_context, std::move(event_bus), std::move(lobby_registry));
    manager->Initialize();

    return manager;
}
void GameManager::Initialize() {
    subscribeWeak<domain::StartGameRequestEvent>(&GameManager::onStartGameRequestEvent);
    subscribeWeak<core::GameTickEvent>(&GameManager::onGameTickEvent);
    subscribeWeak<domain::MoveRequestEvent>(&GameManager::onMoveRequestEvent);
    subscribeWeak<domain::AtackRequestEvent>(&GameManager::onAttackRequestEvent);
}
void GameManager::onStartGameRequestEvent(const domain::StartGameRequestEvent& event) {
    auto player_id = event.player_id;
    if (auto lobby_id_opt = lobby_registry_->getPlayerLobby(player_id); lobby_id_opt.has_value()) {
        auto lobby_id = lobby_id_opt.value();
        auto lobby = lobby_registry_->findLobby(lobby_id);
        // TODO: update start-game logic
        // if (lobby && lobby->checkAllReady() && lobby->getLeader() == player_id) {
        if (lobby) {
            if (dungeon_registry_.addDungeon(domain::GameMap{}, lobby->getAllPlayers())) {
                for (const auto& pid : lobby->getAllPlayers()) {
                    lobby_registry_->removePlayerFromLobby(pid);
                    LOG_INFO(std::format("[GameManager] Player {} removed from lobby {}", pid.value, lobby_id.value));
                }
                LOG_INFO(std::format("[GameManager] Starting game for player {} in lobby {}",
                                     player_id.value,
                                     lobby_id.value));
            }
        } else {
            LOG_ERROR(std::format("[GameManager] Lobby {} not found for player {}", lobby_id.value, player_id.value));
        }
    } else {
        LOG_ERROR(std::format("[GameManager] Player {} is not in any lobby", player_id.value));
    }
}

void GameManager::onGameTickEvent(const core::GameTickEvent& event) {
    std::vector<domain::GameId> dungeons_to_remove;
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(event.timestamp.time_since_epoch()).count();
    for (auto& [game_id, dungeon] : dungeon_registry_.getAllDungeons()) {
        LOG_INFO(std::format("[GameManager] Processing tick for dungeon #{}", game_id.value));
        auto dungeon_state = dungeon->processTick(std::chrono::milliseconds(milliseconds));
        if (!dungeon_state.has_value()) {
            LOG_INFO(std::format("[GameManager] Dungeon {} is over. Removing it from registry.", game_id.value));
            for (const auto& pid : dungeon->getPlayers()) {
                postEvent<domain::GameOverEvent>(pid);
                LOG_INFO(std::format("[GameManager] Published GameOverEvent for player {} in dungeon {}",
                                     pid.value,
                                     game_id.value));
            }
            dungeons_to_remove.push_back(game_id);
            continue;
        }
        postEvent<domain::GameStateUpdateEvent>(game_id, dungeon_state.value());
        LOG_INFO(std::format("[GameManager] Published GameStateUpdateEvent for dungeon {}", game_id.value));
    }
    for (const auto& game_id : dungeons_to_remove) {
        if (!dungeon_registry_.removeDungeon(game_id)) {
            LOG_ERROR(std::format("[GameManager] Failed to remove dungeon #{}", game_id.value));
        }
    }
}

void GameManager::onMoveRequestEvent(const domain::MoveRequestEvent& event) {
    auto player_id = event.player_id;
    auto direction = event.direction;
    std::string dir_str{};
    if (auto dir = directionToString(direction)) {
        dir_str = *dir;
    }
    if (auto game_id_opt = dungeon_registry_.findPlayerDungeon(player_id); game_id_opt.has_value()) {
        auto game_id = game_id_opt.value();
        if (auto dungeon = dungeon_registry_.findDungeon(game_id); dungeon) {
            dungeon->addMovePlayerCommand(player_id, direction);
            LOG_INFO(std::format("[GameManager] Player {} moved in dungeon #{} to direction {}",
                                 player_id.value,
                                 game_id.value,
                                 dir_str));
        } else {
            LOG_ERROR(std::format("[GameManager] Dungeon {} not found for player {}", game_id.value, player_id.value));
        }
    } else {
        LOG_ERROR(std::format("[GameManager] Player {} is not in any dungeon", player_id.value));
    }
}

void GameManager::onAttackRequestEvent(const domain::AtackRequestEvent& event) {
    auto player_id = event.player_id;
    static auto attack = common::utility::getSettings().gameplay.player_default_attack;
    if (auto game_id_opt = dungeon_registry_.findPlayerDungeon(player_id); game_id_opt.has_value()) {
        auto game_id = game_id_opt.value();
        if (auto dungeon = dungeon_registry_.findDungeon(game_id); dungeon) {
            dungeon->addPlayerAttackCommand(player_id, attack);
            LOG_INFO(std::format("[GameManager] Player {} attacked in dungeon {}", player_id.value, game_id.value));
        } else {
            LOG_ERROR(std::format("[GameManager] Dungeon {} not found for player {}", game_id.value, player_id.value));
        }
    } else {
        LOG_ERROR(std::format("[GameManager] Player {} is not in any dungeon", player_id.value));
    }
}

}  // namespace dungeons::server::app
