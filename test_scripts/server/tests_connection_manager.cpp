#include <boost/asio.hpp>
#include <chrono>
#include <common/utility/config.h>
#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <server/app/connection_manager.h>
#include <server/app/session_registry.h>
#include <server/core/event_bus.h>
#include <server/network/session.h>
#include <thread>

using ::testing::_;

using namespace dungeons::server;
using namespace dungeons::server::app;
using namespace dungeons::server::domain;
using namespace dungeons::server::network;

class ConnectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        event_bus_ = core::EventBus::create();
        registry_ = std::make_shared<SessionRegistry>();
        cm_ = ConnectionManager::Create(event_bus_, registry_);
    }

    void TearDown() override {}

    std::shared_ptr<Session> createTestSession(SessionId sid) {
        boost::asio::ip::tcp::socket socket(io_);
        return Session::create(std::move(socket), *event_bus_, sid);
    }

    boost::asio::io_context io_;
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<SessionRegistry> registry_;
    std::shared_ptr<ConnectionManager> cm_;
};

TEST_F(ConnectionManagerTest, HandleClientConnectedEventAddsSession) {
    auto session = createTestSession(SessionId(123));
    EXPECT_EQ(registry_->size(), 0);

    event_bus_->publish(ClientConnectedEvent(session));

    EXPECT_EQ(registry_->size(), 1);
    EXPECT_TRUE(registry_->findSessionBySessionId(SessionId(123)) != nullptr);
}

TEST_F(ConnectionManagerTest, HandleClientConnectedEventAddSessionFailsWhenAlreadyExists) {
    auto session = createTestSession(SessionId(456));
    registry_->addSession(session);
    EXPECT_EQ(registry_->size(), 1);

    event_bus_->publish(ClientConnectedEvent(session));

    EXPECT_EQ(registry_->size(), 1);
}

TEST_F(ConnectionManagerTest, HandleClientDisconnectedEventRemovesSession) {
    auto session = createTestSession(SessionId(789));
    registry_->addSession(session);
    EXPECT_EQ(registry_->size(), 1);

    event_bus_->publish(ClientDisconnectedEvent(session));

    EXPECT_EQ(registry_->size(), 0);
    EXPECT_FALSE(registry_->findSessionBySessionId(SessionId(789)));
}

TEST_F(ConnectionManagerTest, HandleClientDisconnectedEventStoresPlayerId) {
    PlayerId pid(42);
    SessionId sid(111);
    auto session = createTestSession(sid);
    registry_->addSession(session);
    registry_->bindPlayerToSession(pid, sid);
    EXPECT_TRUE(registry_->getPlayerIdBySessionId(sid).has_value());

    event_bus_->publish(ClientDisconnectedEvent(session));

    EXPECT_EQ(registry_->size(), 0);

    auto new_session = createTestSession(sid);
    registry_->addSession(new_session);

    event_bus_->publish(ReconnectRequestedEvent(sid, pid));

    // Проверяем, что переподключение успешно
    auto opt_pid = registry_->getPlayerIdBySessionId(sid);
    EXPECT_TRUE(opt_pid.has_value());
    EXPECT_EQ(opt_pid.value(), pid);
}

TEST_F(ConnectionManagerTest, HandleClientDisconnectedEventNoPlayerId) {
    auto session = createTestSession(SessionId(222));
    registry_->addSession(session);

    event_bus_->publish(ClientDisconnectedEvent(session));

    EXPECT_EQ(registry_->size(), 0);

    PlayerId pid(999);
    event_bus_->publish(ReconnectRequestedEvent(SessionId(222), pid));

    EXPECT_FALSE(registry_->getPlayerIdBySessionId(SessionId(222)).has_value());
}

TEST_F(ConnectionManagerTest, ReconnectRequestedPlayerNotFound) {
    PlayerId pid(100);
    SessionId sid(200);

    event_bus_->publish(ReconnectRequestedEvent(sid, pid));

    EXPECT_EQ(registry_->size(), 0);
    EXPECT_FALSE(registry_->getPlayerIdBySessionId(sid).has_value());
}

TEST_F(ConnectionManagerTest, ReconnectRequestedTimeoutExceeded) {
    PlayerId pid(101);
    SessionId sid(202);

    auto session = createTestSession(sid);
    registry_->addSession(session);
    registry_->bindPlayerToSession(pid, sid);

    event_bus_->publish(ClientDisconnectedEvent(session));
    EXPECT_EQ(registry_->size(), 0);

    auto& cfg = dungeons::common::utility::getSettings();
    std::this_thread::sleep_for(cfg.server.client_disconnect_timeout + std::chrono::seconds(1));

    auto new_session = createTestSession(sid);
    registry_->addSession(new_session);

    event_bus_->publish(ReconnectRequestedEvent(sid, pid));

    EXPECT_FALSE(registry_->getPlayerIdBySessionId(sid).has_value());
}

TEST_F(ConnectionManagerTest, ReconnectRequestedSuccess) {
    PlayerId pid(102);
    SessionId sid(203);

    auto session = createTestSession(sid);
    registry_->addSession(session);
    registry_->bindPlayerToSession(pid, sid);

    event_bus_->publish(ClientDisconnectedEvent(session));
    EXPECT_EQ(registry_->size(), 0);

    auto new_session = createTestSession(sid);
    registry_->addSession(new_session);

    event_bus_->publish(ReconnectRequestedEvent(sid, pid));

    auto opt_pid = registry_->getPlayerIdBySessionId(sid);
    EXPECT_TRUE(opt_pid.has_value());
    EXPECT_EQ(opt_pid.value(), pid);
    auto opt_sid = registry_->getSessionIdByPlayerId(pid);
    EXPECT_TRUE(opt_sid.has_value());
    EXPECT_EQ(opt_sid.value(), sid);
}

TEST_F(ConnectionManagerTest, ReconnectRequestedBindFails) {
    PlayerId pid(103);
    SessionId sid(204);

    auto session = createTestSession(sid);
    registry_->addSession(session);
    registry_->bindPlayerToSession(pid, sid);

    event_bus_->publish(ClientDisconnectedEvent(session));
    EXPECT_EQ(registry_->size(), 0);

    auto new_session = createTestSession(sid);
    registry_->addSession(new_session);

    PlayerId other_pid(999);
    registry_->bindPlayerToSession(other_pid, sid);

    event_bus_->publish(ReconnectRequestedEvent(sid, pid));

    auto opt_pid = registry_->getPlayerIdBySessionId(sid);
    EXPECT_TRUE(opt_pid.has_value());
    EXPECT_EQ(opt_pid.value(), other_pid);
}

TEST_F(ConnectionManagerTest, AuthRequestedSuccess) {
    PlayerId pid(200);
    SessionId sid(300);
    auto session = createTestSession(sid);
    registry_->addSession(session);

    event_bus_->publish(AuthRequestedEvent(sid, pid));

    auto opt_pid = registry_->getPlayerIdBySessionId(sid);
    EXPECT_TRUE(opt_pid.has_value());
    EXPECT_EQ(opt_pid.value(), pid);
    auto opt_sid = registry_->getSessionIdByPlayerId(pid);
    EXPECT_TRUE(opt_sid.has_value());
    EXPECT_EQ(opt_sid.value(), sid);
}

TEST_F(ConnectionManagerTest, AuthRequestedFail) {
    PlayerId pid(201);
    SessionId sid(301);

    event_bus_->publish(AuthRequestedEvent(sid, pid));

    EXPECT_FALSE(registry_->getPlayerIdBySessionId(sid).has_value());
    EXPECT_FALSE(registry_->getSessionIdByPlayerId(pid).has_value());
}

TEST_F(ConnectionManagerTest, AuthRequestedSessionAlreadyBound) {
    PlayerId pid1(300);
    PlayerId pid2(301);
    SessionId sid(400);
    auto session = createTestSession(sid);
    registry_->addSession(session);
    registry_->bindPlayerToSession(pid1, sid);

    event_bus_->publish(AuthRequestedEvent(sid, pid2));

    auto opt_pid = registry_->getPlayerIdBySessionId(sid);
    EXPECT_TRUE(opt_pid.has_value());
    EXPECT_EQ(opt_pid.value(), pid1);
}
