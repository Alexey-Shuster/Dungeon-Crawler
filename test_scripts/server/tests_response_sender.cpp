#include <boost/asio.hpp>
#include <common/wire/serialization.h>
#include <gtest/gtest.h>
#include <memory>
#include <server/app/events.h>
#include <server/app/response_sender.h>
#include <server/app/session_registry.h>
#include <server/core/event_bus.h>
#include <server/network/session.h>
#include <string>
#include <vector>

using namespace dungeons::server::app;
using namespace dungeons::server::core;
using namespace dungeons::server::domain;
using namespace dungeons::server::network;
using namespace dungeons::common::network;
using namespace dungeons::common::types;
using namespace dungeons::common::wire;

// -----------------------------------------------------------------------------
// TestSession – overrides send to capture the raw message data.
// -----------------------------------------------------------------------------
class TestSession : public Session {
public:
    TestSession(boost::asio::io_context& io, EventBus& bus, SessionId sid)
        : Session(boost::asio::ip::tcp::socket(io), bus, sid) {}

    void send(RawMessage msg) override {
        call_ = true;
        captured_data_ = std::move(msg.buffer);
    }

    bool call_ = false;
    ByteBuffer captured_data_;
};

// -----------------------------------------------------------------------------
// Test fixture
// -----------------------------------------------------------------------------
class ResponseSenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        bus_ = EventBus::create();
        registry_ = std::make_shared<SessionRegistry>();
        sender_ = ResponseSender::create(bus_, registry_);
    }

    std::shared_ptr<TestSession> createAndAddSession(PlayerId player_id, SessionId session_id) {
        auto session = std::make_shared<TestSession>(io_, *bus_, session_id);
        registry_->addSession(session);
        registry_->bindPlayerToSession(player_id, session_id);
        return session;
    }

    boost::asio::io_context io_;
    std::shared_ptr<EventBus> bus_;
    std::shared_ptr<SessionRegistry> registry_;
    std::shared_ptr<ResponseSender> sender_;
};

// -----------------------------------------------------------------------------
// Tests for PlayerAuthenticatedEvent
// -----------------------------------------------------------------------------
TEST_F(ResponseSenderTest, AuthSendsWelcome) {
    auto session = createAndAddSession(PlayerId{47}, SessionId{200});
    PlayerAuthenticatedEvent auth_event(SessionId{200}, PlayerId{47});
    bus_->publish(auth_event);

    EXPECT_TRUE(session->call_);
    auto decoded = deserializeMessageRaw(ByteBuffer(std::move(session->captured_data_)));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(std::holds_alternative<NetworkMessageType>(decoded->type));
    EXPECT_EQ(std::get<NetworkMessageType>(decoded->type), NetworkMessageType::kWelcome);
    ASSERT_EQ(decoded->args.size(), 0);
}

TEST_F(ResponseSenderTest, AuthWelcomeWorksForAnyPlayer) {
    auto session = createAndAddSession(PlayerId{0}, SessionId{200});
    PlayerAuthenticatedEvent auth_event(SessionId{200}, PlayerId{0});
    bus_->publish(auth_event);

    EXPECT_TRUE(session->call_);
    auto decoded = deserializeMessageRaw(ByteBuffer(std::move(session->captured_data_)));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(std::holds_alternative<NetworkMessageType>(decoded->type));
    EXPECT_EQ(std::get<NetworkMessageType>(decoded->type), NetworkMessageType::kWelcome);
    ASSERT_EQ(decoded->args.size(), 0);
}

TEST_F(ResponseSenderTest, AuthWithUnknownSessionDoesNotCrash) {
    PlayerAuthenticatedEvent auth_event(SessionId{999}, PlayerId{42});
    EXPECT_NO_THROW(bus_->publish(auth_event));
}

TEST_F(ResponseSenderTest, AuthHandlesMultiplePlayers) {
    auto session1 = createAndAddSession(PlayerId{1408}, SessionId{300});
    auto session2 = createAndAddSession(PlayerId{13}, SessionId{600});

    PlayerAuthenticatedEvent auth_event1(SessionId{300}, PlayerId{1408});
    bus_->publish(auth_event1);
    EXPECT_TRUE(session1->call_);
    auto decoded1 = deserializeMessageRaw(ByteBuffer(std::move(session1->captured_data_)));
    ASSERT_TRUE(decoded1.has_value());
    ASSERT_TRUE(std::holds_alternative<NetworkMessageType>(decoded1->type));
    EXPECT_EQ(std::get<NetworkMessageType>(decoded1->type), NetworkMessageType::kWelcome);
    ASSERT_EQ(decoded1->args.size(), 0);

    PlayerAuthenticatedEvent auth_event2(SessionId{600}, PlayerId{13});
    bus_->publish(auth_event2);
    EXPECT_TRUE(session2->call_);
    auto decoded2 = deserializeMessageRaw(ByteBuffer(std::move(session2->captured_data_)));
    ASSERT_TRUE(decoded2.has_value());
    ASSERT_TRUE(std::holds_alternative<NetworkMessageType>(decoded2->type));
    EXPECT_EQ(std::get<NetworkMessageType>(decoded2->type), NetworkMessageType::kWelcome);
    ASSERT_EQ(decoded2->args.size(), 0);
}

// -----------------------------------------------------------------------------
// Tests for PlayerReconnectedEvent (CBOR)
// -----------------------------------------------------------------------------
TEST_F(ResponseSenderTest, ReconnectedSendsSerializedMessage) {
    auto session = createAndAddSession(PlayerId{77}, SessionId{202});
    PlayerReconnectedEvent reconn_event(SessionId{202}, PlayerId{77});
    bus_->publish(reconn_event);

    EXPECT_TRUE(session->call_);
    auto decoded = deserializeMessageRaw(ByteBuffer(std::move(session->captured_data_)));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(std::holds_alternative<NetworkMessageType>(decoded->type));
    EXPECT_EQ(std::get<NetworkMessageType>(decoded->type), NetworkMessageType::kReconnected);
    ASSERT_EQ(decoded->args.size(), 0);
}

TEST_F(ResponseSenderTest, ReconnectedWithUnknownSessionDoesNotCrash) {
    PlayerReconnectedEvent reconn_event(SessionId{999}, PlayerId{42});
    EXPECT_NO_THROW(bus_->publish(reconn_event));
}

// -----------------------------------------------------------------------------
// Sanity test for ResponseSender creation
// -----------------------------------------------------------------------------
TEST(ResponseSenderCreateTest, CreateDoesNotCrash) {
    auto event_bus = EventBus::create();
    auto session_registry = std::make_shared<SessionRegistry>();
    auto sender = ResponseSender::create(event_bus, session_registry);
    EXPECT_NE(sender, nullptr);
}
