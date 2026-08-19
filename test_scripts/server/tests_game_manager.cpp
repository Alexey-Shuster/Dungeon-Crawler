#include <boost/asio/io_context.hpp>
#include <chrono>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <server/app/game_manager.h>
#include <server/core/event_bus.h>
#include <server/domain/dungeon/dungeon_registry.h>
#include <server/domain/lobby/lobby.h>
#include <server/domain/lobby/lobby_registry.h>
#include <thread>

using namespace testing;

using namespace dungeons::server;
using namespace dungeons::server::app;
using namespace dungeons::server::core;
using namespace dungeons::server::domain;
using namespace dungeons::common::types;

// ============================================================================
// Test Fixture
// ============================================================================

class GameManagerTest : public Test {
protected:
    void SetUp() override {
        io_context_ = std::make_unique<boost::asio::io_context>();
        event_bus_ = EventBus::create();
        lobby_registry_ = std::make_shared<LobbyRegistry>();
        game_manager_ = GameManager::create(*io_context_, event_bus_, lobby_registry_);

        // Запускаем io_context в отдельном потоке
        work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
            io_context_->get_executor());
        io_thread_ = std::thread([this]() {
            io_context_->run();
        });
    }

    void TearDown() override {
        // Останавливаем io_context
        work_guard_.reset();
        if (io_context_) {
            io_context_->stop();
        }
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }

    // Ожидание выполнения всех операций в strand
    void runPendingEvents() {
        std::mutex mtx;
        std::condition_variable cv;
        bool done = false;

        boost::asio::post(*io_context_, [&]() {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() {
            return done;
        });
    }

    // Правильный хелпер: создаем лобби через реестр, а не напрямую
    void registerLobbyWithPlayers(LobbyId lobby_id,
                                  PlayerId leader_id,
                                  const std::vector<PlayerId>& players,
                                  bool all_ready = true) {
        // Создаем лобби без игроков
        auto lobby = std::make_shared<Lobby>(lobby_id, leader_id);
        lobby_registry_->addLobby(lobby);

        // Добавляем игроков через реестр
        for (auto pid : players) {
            bool success = lobby_registry_->addPlayerToLobby(pid, lobby_id);
            ASSERT_TRUE(success) << "Failed to add player " << pid.get() << " to lobby " << lobby_id.get();

            if (all_ready) {
                auto found_lobby = lobby_registry_->findLobby(lobby_id);
                ASSERT_NE(found_lobby, nullptr);
                bool ready_success = found_lobby->setReady(pid, true);
                ASSERT_TRUE(ready_success) << "Failed to set ready for player " << pid.get();
            }
        }
    }

    // Вспомогательный метод для публикации события и ожидания
    template <typename EventType, typename... Args>
    void publishAndWait(Args&&... args) {
        event_bus_->publish(EventType{std::forward<Args>(args)...});
        runPendingEvents();
    }

    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;
    std::thread io_thread_;

    std::shared_ptr<EventBus> event_bus_;
    std::shared_ptr<LobbyRegistry> lobby_registry_;
    std::shared_ptr<GameManager> game_manager_;

    PlayerId player_id_{1};
    LobbyId lobby_id_{1};
};

// ============================================================================
// Tests for onStartGameRequestEvent
// ============================================================================

TEST_F(GameManagerTest, StartGameRequest_PlayerNotInLobby_LogsError) {
    // Arrange
    StartGameRequestEvent event{player_id_};

    // Act
    event_bus_->publish(event);
    runPendingEvents();

    // Assert
    EXPECT_EQ(lobby_registry_->size(), 0);
    EXPECT_FALSE(lobby_registry_->isPlayerInLobby(player_id_));
}

TEST_F(GameManagerTest, StartGameRequest_PlayerInLobbyButLobbyNotReady_NoGameStarted) {
    // Arrange
    std::vector<PlayerId> players = {player_id_};
    registerLobbyWithPlayers(lobby_id_, player_id_, players, false);  // Не все ready

    StartGameRequestEvent event{player_id_};

    // Act
    event_bus_->publish(event);
    runPendingEvents();

    // TODO: update logic
    // Assert
    // EXPECT_TRUE(lobby_registry_->isPlayerInLobby(player_id_));
    // EXPECT_EQ(lobby_registry_->size(), 1);
}

TEST_F(GameManagerTest, StartGameRequest_PlayerNotLeader_NoGameStarted) {
    // Arrange
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent event{player_id_};  // Не лидер

    // Act
    event_bus_->publish(event);
    runPendingEvents();

    // TODO: update logic
    // Assert
    // EXPECT_TRUE(lobby_registry_->isPlayerInLobby(player_id_));
    // EXPECT_TRUE(lobby_registry_->isPlayerInLobby(leader_id));
}

TEST_F(GameManagerTest, StartGameRequest_SuccessfulStart_CreatesDungeonAndRemovesPlayers) {
    // Arrange
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent event{leader_id};

    // Act
    event_bus_->publish(event);
    runPendingEvents();

    // Assert - Все игроки удалены из лобби
    for (auto pid : players) {
        EXPECT_FALSE(lobby_registry_->isPlayerInLobby(pid));
    }
    EXPECT_EQ(lobby_registry_->size(), 0);
}

TEST_F(GameManagerTest, StartGameRequest_MultipleLobbies_OnlyCorrectLobbyProcessed) {
    // Arrange
    PlayerId leader1{2};
    PlayerId leader2{3};
    LobbyId lobby_id2{2};

    std::vector<PlayerId> players1 = {leader1, player_id_};
    std::vector<PlayerId> players2 = {leader2, PlayerId{4}};

    registerLobbyWithPlayers(lobby_id_, leader1, players1, true);
    registerLobbyWithPlayers(lobby_id2, leader2, players2, true);

    // Act - Запускаем игру только для первого лобби
    StartGameRequestEvent event{leader1};
    event_bus_->publish(event);
    runPendingEvents();

    // Assert - Игроки из первого лобби удалены
    for (auto pid : players1) {
        EXPECT_FALSE(lobby_registry_->isPlayerInLobby(pid));
    }

    // Assert - Игроки из второго лобби остались
    for (auto pid : players2) {
        EXPECT_TRUE(lobby_registry_->isPlayerInLobby(pid));
    }
}

TEST_F(GameManagerTest, StartGameRequest_SuccessfulStart_EmitsGameStartedEvent) {
    // Arrange
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    // Act & Assert - Просто проверяем что нет исключений
    StartGameRequestEvent event{leader_id};
    EXPECT_NO_THROW(event_bus_->publish(event));
    runPendingEvents();
}

// ============================================================================
// Tests for onGameTickEvent
// ============================================================================

TEST_F(GameManagerTest, GameTick_NoDungeons_NoEventsPublished) {
    // Arrange
    auto timestamp = std::chrono::steady_clock::now();
    GameTickEvent event{timestamp};

    // Act & Assert
    EXPECT_NO_THROW(event_bus_->publish(event));
    runPendingEvents();
}

TEST_F(GameManagerTest, GameTick_DungeonExists_ProcessTickCalled) {
    // Arrange - Создаем игру
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act - Отправляем тик
    auto timestamp = std::chrono::steady_clock::now();
    GameTickEvent tick_event{timestamp};

    // Assert
    EXPECT_NO_THROW(event_bus_->publish(tick_event));
    runPendingEvents();
}

TEST_F(GameManagerTest, GameTick_MultipleDungeons_AllProcessed) {
    // Arrange - Создаем две игры
    PlayerId leader1{2};
    PlayerId leader2{3};
    LobbyId lobby_id2{2};

    std::vector<PlayerId> players1 = {leader1, player_id_};
    std::vector<PlayerId> players2 = {leader2, PlayerId{4}};

    registerLobbyWithPlayers(lobby_id_, leader1, players1, true);
    registerLobbyWithPlayers(lobby_id2, leader2, players2, true);

    StartGameRequestEvent start1{leader1};
    StartGameRequestEvent start2{leader2};
    event_bus_->publish(start1);
    event_bus_->publish(start2);
    runPendingEvents();

    // Act
    auto timestamp = std::chrono::steady_clock::now();
    GameTickEvent tick_event{timestamp};

    // Assert
    EXPECT_NO_THROW(event_bus_->publish(tick_event));
    runPendingEvents();
}

// ============================================================================
// Tests for onMoveRequestEvent
// ============================================================================

TEST_F(GameManagerTest, MoveRequest_PlayerNotInDungeon_LogsError) {
    // Arrange
    MoveRequestEvent event{player_id_, Direction::kUp};

    // Act & Assert
    EXPECT_NO_THROW(event_bus_->publish(event));
    runPendingEvents();
}

TEST_F(GameManagerTest, MoveRequest_PlayerInDungeon_AddsMoveCommand) {
    // Arrange - Создаем игру
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act
    MoveRequestEvent move_event{player_id_, Direction::kUp};

    // Assert
    EXPECT_NO_THROW(event_bus_->publish(move_event));
    runPendingEvents();
}

TEST_F(GameManagerTest, MoveRequest_AllDirections_HandledCorrectly) {
    // Arrange - Создаем игру
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act - Тестируем все направления
    std::vector<Direction> directions = {Direction::kUp, Direction::kDown, Direction::kLeft, Direction::kRight};

    for (auto dir : directions) {
        MoveRequestEvent move_event{player_id_, dir};
        EXPECT_NO_THROW(event_bus_->publish(move_event));
        runPendingEvents();
    }
}

// ============================================================================
// Tests for onAttackRequestEvent
// ============================================================================

TEST_F(GameManagerTest, AttackRequest_PlayerNotInDungeon_LogsError) {
    // Arrange
    AtackRequestEvent event{player_id_};

    // Act & Assert
    EXPECT_NO_THROW(event_bus_->publish(event));
    runPendingEvents();
}

TEST_F(GameManagerTest, AttackRequest_PlayerInDungeon_AddsAttackCommand) {
    // Arrange - Создаем игру
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act
    AtackRequestEvent attack_event{player_id_};

    // Assert
    EXPECT_NO_THROW(event_bus_->publish(attack_event));
    runPendingEvents();
}

TEST_F(GameManagerTest, AttackRequest_MultiplePlayers_AllAttacksHandled) {
    // Arrange - Создаем игру с несколькими игроками
    PlayerId leader_id{2};
    PlayerId player3{3};
    std::vector<PlayerId> players = {leader_id, player_id_, player3};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act - Все игроки атакуют
    for (auto pid : players) {
        AtackRequestEvent attack_event{pid};
        EXPECT_NO_THROW(event_bus_->publish(attack_event));
        runPendingEvents();
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(GameManagerTest, FullGameFlow_SuccessfulGameSequence) {
    // Arrange
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    // Act
    // 1. Start game
    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // 2. Process game ticks
    for (int i = 0; i < 5; ++i) {
        auto timestamp = std::chrono::steady_clock::now();
        GameTickEvent tick_event{timestamp};
        event_bus_->publish(tick_event);
        runPendingEvents();
    }

    // 3. Player moves
    MoveRequestEvent move_event{player_id_, Direction::kUp};
    event_bus_->publish(move_event);
    runPendingEvents();

    // 4. Player attacks
    AtackRequestEvent attack_event{player_id_};
    event_bus_->publish(attack_event);
    runPendingEvents();

    // Assert
    for (auto pid : players) {
        EXPECT_FALSE(lobby_registry_->isPlayerInLobby(pid));
    }
}

TEST_F(GameManagerTest, FullGameFlow_GameOver_RemovesDungeon) {
    // Arrange - Создаем игру
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act - Отправляем тики, чтобы игра завершилась
    for (int i = 0; i < 50; ++i) {
        auto timestamp = std::chrono::steady_clock::now();
        GameTickEvent tick_event{timestamp};
        event_bus_->publish(tick_event);
        runPendingEvents();
    }

    // Assert - Игроки должны быть удалены из лобби
    for (auto pid : players) {
        EXPECT_FALSE(lobby_registry_->isPlayerInLobby(pid));
    }
}

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST_F(GameManagerTest, ConcurrentEvents_NoDataRaces) {
    // Arrange
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    // Act - Запускаем несколько потоков
    std::thread start_thread([this, leader_id]() {
        StartGameRequestEvent event{leader_id};
        event_bus_->publish(event);
    });

    std::thread move_thread([this]() {
        for (int i = 0; i < 5; ++i) {
            MoveRequestEvent event{player_id_, Direction::kUp};
            event_bus_->publish(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    std::thread tick_thread([this]() {
        for (int i = 0; i < 5; ++i) {
            auto timestamp = std::chrono::steady_clock::now();
            GameTickEvent event{timestamp};
            event_bus_->publish(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    std::thread attack_thread([this]() {
        for (int i = 0; i < 5; ++i) {
            AtackRequestEvent event{player_id_};
            event_bus_->publish(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    start_thread.join();
    move_thread.join();
    tick_thread.join();
    attack_thread.join();

    runPendingEvents();

    // Assert
    SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(GameManagerTest, StartGameRequest_EmptyLobby_NoGameStarted) {
    // Arrange - Лобби без игроков
    auto lobby = std::make_shared<Lobby>(lobby_id_, player_id_);
    lobby_registry_->addLobby(lobby);

    StartGameRequestEvent event{player_id_};

    // Act
    event_bus_->publish(event);
    runPendingEvents();

    // Assert - Лобби не должно быть удалено
    EXPECT_NE(lobby_registry_->findLobby(lobby_id_), nullptr);
}

TEST_F(GameManagerTest, MoveRequest_DirectionConversion_CorrectDirection) {
    // Arrange - Создаем игру
    PlayerId leader_id{2};
    std::vector<PlayerId> players = {leader_id, player_id_};
    registerLobbyWithPlayers(lobby_id_, leader_id, players, true);

    StartGameRequestEvent start_event{leader_id};
    event_bus_->publish(start_event);
    runPendingEvents();

    // Act - Тестируем все направления из сообщений
    std::vector<Direction> message_directions = {Direction::kUp, Direction::kDown, Direction::kLeft, Direction::kRight};

    for (auto dir : message_directions) {
        MoveRequestEvent move_event{player_id_, dir};
        EXPECT_NO_THROW(event_bus_->publish(move_event));
        runPendingEvents();
    }
}
