#include <boost/asio.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <server/core/event_bus.h>
#include <server/network/session.h>

using namespace boost::asio;
using namespace dungeons::server;
using namespace dungeons::server::network;
using namespace dungeons::common::network;

struct DummySession {
    io_context io;
    ip::tcp::socket dummy_socket;
    std::shared_ptr<core::EventBus> bus;
    std::shared_ptr<Session> session;

    DummySession()
        : dummy_socket(io)
        , bus(core::EventBus::create())
        , session(Session::create(std::move(dummy_socket), *bus, SessionId{0})) {}
};

// ---------------------------------------------------------------------
// Basic construction and identifier tests
// ---------------------------------------------------------------------

TEST(SessionSimpleTest, DefaultSessionIdIsZero) {
    DummySession ds;
    EXPECT_EQ(ds.session->getSessionId(), SessionId{0});
}

TEST(SessionSimpleTest, SessionIdCanBeSpecified) {
    io_context io;
    ip::tcp::socket sock(io);
    auto bus = core::EventBus::create();
    auto sess = Session::create(std::move(sock), *bus, SessionId{123});
    EXPECT_EQ(sess->getSessionId(), SessionId{123});
}

// ---------------------------------------------------------------------
// Disconnect handling
// ---------------------------------------------------------------------

TEST(SessionSimpleTest, HandleDisconnectDoesNotCrashWhenCalledOnce) {
    DummySession ds;
    EXPECT_NO_THROW(ds.session->handleDisconnect());
}

TEST(SessionSimpleTest, HandleDisconnectCanBeCalledTwiceWithoutCrash) {
    DummySession ds;
    ds.session->handleDisconnect();
    EXPECT_NO_THROW(ds.session->handleDisconnect());
}

// ---------------------------------------------------------------------
// send() behaviour in different states
// ---------------------------------------------------------------------

TEST(SessionSimpleTest, SendDoesNothingIfDisconnectedBeforeStart) {
    DummySession ds;
    ds.session->handleDisconnect();
    std::string msg = "test message";
    std::vector<uint8_t> bytes(msg.begin(), msg.end());
    EXPECT_NO_THROW(ds.session->send(RawMessage{std::move(bytes)}));
}

TEST(SessionSimpleTest, SendBeforeDisconnectDoesNotCrash) {
    DummySession ds;
    ds.session->start();  // start the session (read loop)
    std::string msg = "hello";
    std::vector<uint8_t> bytes(msg.begin(), msg.end());
    EXPECT_NO_THROW(ds.session->send(RawMessage{std::move(bytes)}));
    ds.io.run_for(std::chrono::milliseconds(10));  // let any async ops finish
}

TEST(SessionSimpleTest, MultipleSendsDoNotCrash) {
    DummySession ds;
    ds.session->start();
    for (int i = 0; i < 10; ++i) {
        std::string msg = "message " + std::to_string(i);
        std::vector<uint8_t> bytes(msg.begin(), msg.end());
        EXPECT_NO_THROW(ds.session->send(RawMessage{std::move(bytes)}));
    }
    ds.io.run_for(std::chrono::milliseconds(10));
}

TEST(SessionSimpleTest, SendAfterDisconnectDoesNothing) {
    DummySession ds;
    ds.session->start();
    ds.io.run_for(std::chrono::milliseconds(10));  // allow the read loop to start
    ds.session->handleDisconnect();                // disconnect
    std::string msg = "should be ignored";
    std::vector<uint8_t> bytes(msg.begin(), msg.end());
    EXPECT_NO_THROW(ds.session->send(RawMessage{std::move(bytes)}));  // returns immediately, no exception
}

// ---------------------------------------------------------------------
// Start method
// ---------------------------------------------------------------------

TEST(SessionSimpleTest, StartDoesNotCrashWithDummySocket) {
    DummySession ds;
    EXPECT_NO_THROW(ds.session->start());
    ds.io.run_for(std::chrono::milliseconds(10));
}
