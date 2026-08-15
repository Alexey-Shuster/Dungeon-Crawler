#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <server/app/session_registry.h>
#include <server/core/event_bus.h>
#include <server/network/session.h>

using namespace dungeons::server;
using namespace dungeons::server::app;
using namespace dungeons::server::domain;
using namespace dungeons::server::network;

// Вспомогательная фабрика для создания сессий в тестах
static std::shared_ptr<Session> makeSession(boost::asio::io_context& io, core::EventBus& bus, SessionId sid) {
    return Session::create(boost::asio::ip::tcp::socket(io), bus, sid);
}

class SessionRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = std::make_shared<app::SessionRegistry>();
        bus = core::EventBus::create();
    }

    void TearDown() override {
        io.stop();
    }

    boost::asio::io_context io;
    std::shared_ptr<core::EventBus> bus;
    std::shared_ptr<app::SessionRegistry> registry;
};

TEST_F(SessionRegistryTest, AddAndFind) {
    SessionId sid(1);
    auto s = makeSession(io, *bus, sid);
    EXPECT_TRUE(registry->addSession(s));
    EXPECT_EQ(registry->findSessionBySessionId(sid), s);
    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
}

TEST_F(SessionRegistryTest, RemoveAndGone) {
    SessionId sid(2);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_EQ(registry->findSessionBySessionId(sid), nullptr);
    EXPECT_EQ(registry->size(), 0);
}

TEST_F(SessionRegistryTest, SizeChanges) {
    SessionId sid(3);
    auto s = makeSession(io, *bus, sid);
    EXPECT_EQ(registry->size(), 0);
    registry->addSession(s);
    EXPECT_EQ(registry->size(), 1);
    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_EQ(registry->size(), 0);
}

TEST_F(SessionRegistryTest, AddDuplicateSession) {
    SessionId sid(4);
    auto s1 = makeSession(io, *bus, sid);
    auto s2 = makeSession(io, *bus, sid);

    EXPECT_TRUE(registry->addSession(s1));
    EXPECT_FALSE(registry->addSession(s2));
    EXPECT_EQ(registry->size(), 1);
    EXPECT_EQ(registry->findSessionBySessionId(sid), s1);
}

TEST_F(SessionRegistryTest, RemoveNonExistentSession) {
    SessionId sid(999);
    EXPECT_FALSE(registry->removeSessionBySessionId(sid));
    EXPECT_EQ(registry->size(), 0);
}

TEST_F(SessionRegistryTest, FindNonExistentSession) {
    SessionId sid(999);
    EXPECT_EQ(registry->findSessionBySessionId(sid), nullptr);
}

TEST_F(SessionRegistryTest, GetAllSessions) {
    SessionId sid1(10), sid2(20), sid3(30);
    auto s1 = makeSession(io, *bus, sid1);
    auto s2 = makeSession(io, *bus, sid2);
    auto s3 = makeSession(io, *bus, sid3);

    registry->addSession(s1);
    registry->addSession(s2);
    registry->addSession(s3);

    auto all = registry->getAllSessions();
    EXPECT_EQ(all.size(), 3);

    bool found1 = false, found2 = false, found3 = false;
    for (const auto& s : all) {
        if (s == s1)
            found1 = true;
        if (s == s2)
            found2 = true;
        if (s == s3)
            found3 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
    EXPECT_TRUE(found3);
}

TEST_F(SessionRegistryTest, GetAllSessionsEmpty) {
    auto all = registry->getAllSessions();
    EXPECT_TRUE(all.empty());
}

TEST_F(SessionRegistryTest, BindPlayerToSession) {
    SessionId sid(100);
    PlayerId pid(5);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);

    EXPECT_TRUE(registry->bindPlayerToSession(pid, sid));
    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_EQ(registry->size(), 0);
    EXPECT_FALSE(registry->removeSessionBySessionId(sid));
}

TEST_F(SessionRegistryTest, BindPlayerToNonExistentSession) {
    PlayerId pid(5);
    SessionId sid(999);
    EXPECT_FALSE(registry->bindPlayerToSession(pid, sid));
}

TEST_F(SessionRegistryTest, BindMultiplePlayers) {
    SessionId sid1(200), sid2(300);
    PlayerId pid1(10), pid2(20);
    auto s1 = makeSession(io, *bus, sid1);
    auto s2 = makeSession(io, *bus, sid2);

    registry->addSession(s1);
    registry->addSession(s2);

    EXPECT_TRUE(registry->bindPlayerToSession(pid1, sid1));
    EXPECT_TRUE(registry->bindPlayerToSession(pid2, sid2));

    EXPECT_TRUE(registry->removeSessionBySessionId(sid1));
    EXPECT_EQ(registry->size(), 1);
    EXPECT_EQ(registry->findSessionBySessionId(sid2), s2);
}

TEST_F(SessionRegistryTest, RebindPlayerToAnotherSession) {
    SessionId sid1(400), sid2(500);
    PlayerId pid(30);
    auto s1 = makeSession(io, *bus, sid1);
    auto s2 = makeSession(io, *bus, sid2);

    registry->addSession(s1);
    registry->addSession(s2);

    EXPECT_TRUE(registry->bindPlayerToSession(pid, sid1));
    EXPECT_TRUE(registry->bindPlayerToSession(pid, sid2));

    EXPECT_TRUE(registry->removeSessionBySessionId(sid1));
    EXPECT_EQ(registry->size(), 1);
    EXPECT_EQ(registry->findSessionBySessionId(sid2), s2);
}

TEST_F(SessionRegistryTest, RemoveSessionWithBoundPlayer) {
    SessionId sid(600);
    PlayerId pid(40);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_EQ(registry->size(), 0);
    EXPECT_FALSE(registry->removeSessionBySessionId(sid));
}

TEST_F(SessionRegistryTest, AddAfterRemoveWithSameId) {
    SessionId sid(700);
    auto s1 = makeSession(io, *bus, sid);
    registry->addSession(s1);
    EXPECT_TRUE(registry->removeSessionBySessionId(sid));

    auto s2 = makeSession(io, *bus, sid);
    EXPECT_TRUE(registry->addSession(s2));
    EXPECT_EQ(registry->findSessionBySessionId(sid), s2);
}

TEST_F(SessionRegistryTest, MultipleOperations) {
    std::vector<std::shared_ptr<Session>> sessions;
    for (int i = 0; i < 10; ++i) {
        SessionId sid(i + 1000);
        auto s = makeSession(io, *bus, sid);
        sessions.push_back(s);
        registry->addSession(s);
    }

    EXPECT_EQ(registry->size(), 10);

    for (int i = 0; i < 5; ++i) {
        SessionId sid(i + 1000);
        EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    }

    EXPECT_EQ(registry->size(), 5);

    auto all = registry->getAllSessions();
    EXPECT_EQ(all.size(), 5);
}

TEST_F(SessionRegistryTest, IsPlayerIdBound) {
    SessionId sid(1000);
    PlayerId pid(42);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);

    EXPECT_FALSE(registry->isPlayerIdBound(pid));

    EXPECT_TRUE(registry->bindPlayerToSession(pid, sid));
    EXPECT_TRUE(registry->isPlayerIdBound(pid));

    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_FALSE(registry->isPlayerIdBound(pid));
}

TEST_F(SessionRegistryTest, IsSessionIdBound) {
    SessionId sid(2000);
    PlayerId pid(100);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);

    EXPECT_FALSE(registry->isSessionIdBound(sid));

    EXPECT_TRUE(registry->bindPlayerToSession(pid, sid));
    EXPECT_TRUE(registry->isSessionIdBound(sid));

    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_FALSE(registry->isSessionIdBound(sid));
}

TEST_F(SessionRegistryTest, FindSessionByPlayerId) {
    SessionId sid(3000);
    PlayerId pid(200);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    auto found = registry->findSessionByPlayerId(pid);
    EXPECT_EQ(found, s);
}

TEST_F(SessionRegistryTest, FindSessionByPlayerIdNotFound) {
    PlayerId pid(999);
    auto found = registry->findSessionByPlayerId(pid);
    EXPECT_EQ(found, nullptr);

    SessionId sid(4000);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    PlayerId pid2(300);
    auto found2 = registry->findSessionByPlayerId(pid2);
    EXPECT_EQ(found2, nullptr);
}

TEST_F(SessionRegistryTest, GetPlayerIdBySessionId) {
    SessionId sid(6000);
    PlayerId pid(60);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    auto result = registry->getPlayerIdBySessionId(sid);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), pid);
}

TEST_F(SessionRegistryTest, GetPlayerIdBySessionIdNotFound) {
    SessionId sid(7000);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);

    auto result = registry->getPlayerIdBySessionId(sid);
    EXPECT_FALSE(result.has_value());

    SessionId sid2(8888);
    auto result2 = registry->getPlayerIdBySessionId(sid2);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(SessionRegistryTest, GetSessionIdByPlayerId) {
    SessionId sid(8000);
    PlayerId pid(80);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    auto result = registry->getSessionIdByPlayerId(pid);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), sid);
}

TEST_F(SessionRegistryTest, GetSessionIdByPlayerIdNotFound) {
    SessionId sid(9000);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);

    PlayerId pid(90);
    auto result = registry->getSessionIdByPlayerId(pid);
    EXPECT_FALSE(result.has_value());

    PlayerId pid2(9999);
    auto result2 = registry->getSessionIdByPlayerId(pid2);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(SessionRegistryTest, RebindAndCheckBound) {
    SessionId sid1(10000), sid2(11000);
    PlayerId pid(7);
    auto s1 = makeSession(io, *bus, sid1);
    auto s2 = makeSession(io, *bus, sid2);

    registry->addSession(s1);
    registry->addSession(s2);

    registry->bindPlayerToSession(pid, sid1);
    EXPECT_TRUE(registry->isPlayerIdBound(pid));
    EXPECT_TRUE(registry->isSessionIdBound(sid1));
    EXPECT_FALSE(registry->isSessionIdBound(sid2));

    registry->removeSessionBySessionId(sid1);

    registry->bindPlayerToSession(pid, sid2);
    EXPECT_TRUE(registry->isPlayerIdBound(pid));
    EXPECT_TRUE(registry->isSessionIdBound(sid2));
    EXPECT_FALSE(registry->isSessionIdBound(sid1));

    auto found = registry->findSessionByPlayerId(pid);
    EXPECT_EQ(found, s2);

    EXPECT_TRUE(registry->removeSessionBySessionId(sid2));
    EXPECT_FALSE(registry->isPlayerIdBound(pid));
    EXPECT_FALSE(registry->isSessionIdBound(sid2));
}

TEST_F(SessionRegistryTest, BoundSessionRemoved) {
    SessionId sid(12000);
    PlayerId pid(11);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    EXPECT_TRUE(registry->isSessionIdBound(sid));
    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    EXPECT_FALSE(registry->isSessionIdBound(sid));
    EXPECT_FALSE(registry->isPlayerIdBound(pid));
}

TEST_F(SessionRegistryTest, IsBoundNonExistent) {
    PlayerId pid(9999);
    SessionId sid(8888);
    EXPECT_FALSE(registry->isPlayerIdBound(pid));
    EXPECT_FALSE(registry->isSessionIdBound(sid));
}

TEST_F(SessionRegistryTest, FindSessionByPlayerAfterSessionRemoved) {
    SessionId sid(13000);
    PlayerId pid(13);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    EXPECT_TRUE(registry->removeSessionBySessionId(sid));
    auto found = registry->findSessionByPlayerId(pid);
    EXPECT_EQ(found, nullptr);
    EXPECT_FALSE(registry->isPlayerIdBound(pid));
}

TEST_F(SessionRegistryTest, GetPlayerIdAndSessionIdConsistency) {
    SessionId sid(16000);
    PlayerId pid(16);
    auto s = makeSession(io, *bus, sid);
    registry->addSession(s);
    registry->bindPlayerToSession(pid, sid);

    auto playerId = registry->getPlayerIdBySessionId(sid);
    auto sessionId = registry->getSessionIdByPlayerId(pid);

    EXPECT_TRUE(playerId.has_value());
    EXPECT_TRUE(sessionId.has_value());
    EXPECT_EQ(playerId.value(), pid);
    EXPECT_EQ(sessionId.value(), sid);
}
