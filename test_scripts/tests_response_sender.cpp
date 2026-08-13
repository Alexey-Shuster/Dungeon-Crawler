#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "../common/serialization.h"
#include "../common/types.h"
#include "../server/response_sender.h"
#include "../server/session.h"
#include "core/event_bus.h"
#include "network/session_registry.h"

using namespace network;
using namespace events;

// -----------------------------------------------------------------------------
// TestSession – overrides send to capture the raw message data.
// -----------------------------------------------------------------------------
class TestSession : public Session {
public:
    TestSession(boost::asio::io_context& io, EventBus& bus, SessionId sid) :
        Session(boost::asio::ip::tcp::socket(io), bus, sid) {}

    void send(MessageData&& raw_message) override {
        call_ = true;
        captured_data_ = std::move(raw_message);
    }

    bool call_ = false;
    MessageData captured_data_;
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
    auto decoded = serialization::deserializeMessageRaw(Message(std::move(session->captured_data_)));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(std::holds_alternative<message::NetworkMessageType>(decoded->type));
    EXPECT_EQ(std::get<message::NetworkMessageType>(decoded->type), message::NetworkMessageType::kWelcome);
    ASSERT_EQ(decoded->args.size(), 0);
}

TEST_F(ResponseSenderTest, AuthWelcomeWorksForAnyPlayer) {
    auto session = createAndAddSession(PlayerId{0}, SessionId{200});
    PlayerAuthenticatedEvent auth_event(SessionId{200}, PlayerId{0});
    bus_->publish(auth_event);

    EXPECT_TRUE(session->call_);
    auto decoded = serialization::deserializeMessageRaw(Message(std::move(session->captured_data_)));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(std::holds_alternative<message::NetworkMessageType>(decoded->type));
    EXPECT_EQ(std::get<message::NetworkMessageType>(decoded->type), message::NetworkMessageType::kWelcome);
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
    auto decoded1 = serialization::deserializeMessageRaw(Message(std::move(session1->captured_data_)));
    ASSERT_TRUE(decoded1.has_value());
    ASSERT_TRUE(std::holds_alternative<message::NetworkMessageType>(decoded1->type));
    EXPECT_EQ(std::get<message::NetworkMessageType>(decoded1->type), message::NetworkMessageType::kWelcome);
    ASSERT_EQ(decoded1->args.size(), 0);

    PlayerAuthenticatedEvent auth_event2(SessionId{600}, PlayerId{13});
    bus_->publish(auth_event2);
    EXPECT_TRUE(session2->call_);
    auto decoded2 = serialization::deserializeMessageRaw(Message(std::move(session2->captured_data_)));
    ASSERT_TRUE(decoded2.has_value());
    ASSERT_TRUE(std::holds_alternative<message::NetworkMessageType>(decoded2->type));
    EXPECT_EQ(std::get<message::NetworkMessageType>(decoded2->type), message::NetworkMessageType::kWelcome);
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
    auto decoded = serialization::deserializeMessageRaw(Message(std::move(session->captured_data_)));
    ASSERT_TRUE(decoded.has_value());
    ASSERT_TRUE(std::holds_alternative<message::NetworkMessageType>(decoded->type));
    EXPECT_EQ(std::get<message::NetworkMessageType>(decoded->type), message::NetworkMessageType::kReconnected);
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
