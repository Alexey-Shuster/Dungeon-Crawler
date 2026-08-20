#include <common/network/raw_message.h>
#include <common/types/message_utils.h>
#include <common/wire/serder.h>
#include <gtest/gtest.h>
#include <server/app/message_dispatcher.h>
#include <server/domain/dungeon/dungeon.h>

using namespace dungeons::server::app;
using namespace dungeons::server::domain;
using namespace dungeons::server::network;
using namespace dungeons::common::network;
using namespace dungeons::common::types;
using namespace dungeons::common::wire;

#define EXPECT_SESSION_EQ(actual, expected) EXPECT_EQ(static_cast<uint64_t>(actual), static_cast<uint64_t>(expected))
#define EXPECT_PLAYER_EQ(actual, expected) EXPECT_EQ(static_cast<uint64_t>(actual), static_cast<uint64_t>(expected))

// Helper to create a RawMessage from a byte vector
RawMessage createRawMessage(std::vector<uint8_t> data) {
    return RawMessage{std::move(data)};
}

// Test message type used throughout
constexpr auto kTestMsgType = dungeons::dc_NetMsg::kJoin;

// ============================================================================
// makeEvent tests (no CBOR)
// ============================================================================

class MakeEventTest : public ::testing::Test {
protected:
    using MessageArgs = MessageArgs;
};

TEST_F(MakeEventTest, CreateAuthRequestedEvent) {
    MessageArgs args = std::vector<uint64_t>{123, 456};

    auto event = makeEvent(kTestMsgType, args);

    ASSERT_NE(event, nullptr);
    auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);
    EXPECT_SESSION_EQ(auth_event->session_id.get(), 123);
    EXPECT_PLAYER_EQ(auth_event->player_id.get(), 456);
}

TEST_F(MakeEventTest, UnknownMessageTypeReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{123, 456};
    auto event = makeEvent(std::monostate{}, args);
    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, InvalidArgsCountReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{123};
    auto event = makeEvent(kTestMsgType, args);
    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, EmptyArgsReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{};
    auto event = makeEvent(kTestMsgType, args);
    EXPECT_EQ(event, nullptr);
}

// ============================================================================
// Integration tests using public serialization / deserialization
// ============================================================================

class SerializationIntegrationTest : public ::testing::Test {};

TEST_F(SerializationIntegrationTest, SerializeDeserializeJoinMessage) {
    SessionId session_id{123};
    PlayerId player_id{456};

    auto buffer = serializeMessageToBuffer(kTestMsgType, session_id.get(), player_id.get());
    ASSERT_TRUE(buffer.has_value());
    auto buffer_copy = buffer;

    auto raw = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(raw.has_value());

    // Use helpers from message_utils.h
    ASSERT_TRUE(isNetworkMessage(raw->type));
    auto netMsg = asNetworkMessage(raw->type);
    ASSERT_TRUE(netMsg.has_value());
    EXPECT_EQ(*netMsg, kTestMsgType);

    ASSERT_EQ(raw->args.size(), 2);
    EXPECT_EQ(raw->args[0], 123);
    EXPECT_EQ(raw->args[1], 456);

    // Also test full pipeline via deserializeMessage (which uses makeEvent)
    auto event = deserializeMessage(createRawMessage(std::move(*buffer_copy)));
    ASSERT_NE(event, nullptr);
    auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);
    EXPECT_SESSION_EQ(auth_event->session_id.get(), 123);
    EXPECT_PLAYER_EQ(auth_event->player_id.get(), 456);
}

TEST_F(SerializationIntegrationTest, SerializeDeserializeWithLargeIds) {
    SessionId session_id{0xFFFFFFFFFFFFFFFF};
    PlayerId player_id{0x0123456789ABCDEF};

    auto buffer = serializeMessageToBuffer(kTestMsgType, session_id.get(), player_id.get());
    ASSERT_TRUE(buffer.has_value());

    auto raw = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(raw.has_value());

    ASSERT_TRUE(isNetworkMessage(raw->type));
    auto netMsg = asNetworkMessage(raw->type);
    ASSERT_TRUE(netMsg.has_value());
    EXPECT_EQ(*netMsg, kTestMsgType);

    ASSERT_EQ(raw->args.size(), 2);
    EXPECT_EQ(raw->args[0], 0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ(raw->args[1], 0x0123456789ABCDEF);
}

TEST_F(SerializationIntegrationTest, SerializeUnknownMessageTypeReturnsNullopt) {
    auto buffer = serializeMessageToBuffer(std::monostate{}, SessionId{123}.get(), PlayerId{456}.get());
    EXPECT_FALSE(buffer.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeInvalidDataReturnsNull) {
    std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02};
    RawMessage msg = createRawMessage(invalid_data);
    auto raw = deserializeBufferToMessage(std::move(msg.buffer));
    EXPECT_FALSE(raw.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeEmptyMessageReturnsNull) {
    RawMessage msg = createRawMessage({});
    auto raw = deserializeBufferToMessage(std::move(msg.buffer));
    EXPECT_FALSE(raw.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeNonArrayReturnsNull) {
    // CBOR encoding of integer 42 (not an array)
    std::vector<uint8_t> data = {0x18, 0x2A};
    RawMessage msg = createRawMessage(data);
    auto raw = deserializeBufferToMessage(std::move(msg.buffer));
    EXPECT_FALSE(raw.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeEmptyArrayReturnsNull) {
    // CBOR empty array: 0x80
    std::vector<uint8_t> data = {0x80};
    RawMessage msg = createRawMessage(data);
    auto raw = deserializeBufferToMessage(std::move(msg.buffer));
    EXPECT_FALSE(raw.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeInvalidMessageTypeReturnsNull) {
    // CBOR array: [ "invalid", 123 ]
    std::vector<uint8_t> data = {0x82, 0x65, 0x69, 0x6E, 0x76, 0x61, 0x6C, 0x69, 0x64, 0x18, 0x7B};
    RawMessage msg = createRawMessage(data);
    auto raw = deserializeBufferToMessage(std::move(msg.buffer));
    EXPECT_FALSE(raw.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeWithInvalidArgsReturnsNull) {
    // CBOR array: [ 0 (type), "not a number" ]
    std::vector<uint8_t> data =
        {0x82, 0x00, 0x6D, 0x6E, 0x6F, 0x74, 0x20, 0x61, 0x20, 0x6E, 0x75, 0x6D, 0x62, 0x65, 0x72};
    RawMessage msg = createRawMessage(data);
    auto raw = deserializeBufferToMessage(std::move(msg.buffer));
    EXPECT_FALSE(raw.has_value());
}

// ============================================================================
// deserializeBufferToMessage public API tests
// ============================================================================

TEST(DeserializeRawTest, ValidMessage) {
    auto buffer = serializeMessageToBuffer(kTestMsgType, SessionId{123}.get(), PlayerId{456}.get());
    ASSERT_TRUE(buffer.has_value());

    auto raw = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(raw.has_value());

    ASSERT_TRUE(isNetworkMessage(raw->type));
    auto netMsg = asNetworkMessage(raw->type);
    ASSERT_TRUE(netMsg.has_value());
    EXPECT_EQ(*netMsg, kTestMsgType);

    ASSERT_EQ(raw->args.size(), 2);
    EXPECT_EQ(raw->args[0], 123);
    EXPECT_EQ(raw->args[1], 456);
}

TEST(DeserializeRawTest, InvalidDataReturnsNullopt) {
    std::vector<uint8_t> invalid = {0xFF, 0xFF};
    auto raw = deserializeBufferToMessage(ByteBuffer{std::move(invalid)});
    EXPECT_FALSE(raw.has_value());
}

// ============================================================================
// Full round‑trip integration with various values
// ============================================================================

TEST(IntegrationTest, RoundTripWithVariousValues) {
    struct TestCase {
        SessionId session_id;
        PlayerId player_id;
    };

    std::vector<TestCase> test_cases = {{SessionId{0}, PlayerId{0}},
                                        {SessionId{1}, PlayerId{1}},
                                        {SessionId{123}, PlayerId{456}},
                                        {SessionId{65535}, PlayerId{65536}},
                                        {SessionId{0xFFFFFFFF}, PlayerId{0xFFFFFFFFFFFFFFFF}},
                                        {SessionId{0x0123456789ABCDEF}, PlayerId{0xFEDCBA9876543210}}};

    for (const auto& tc : test_cases) {
        auto buffer = serializeMessageToBuffer(kTestMsgType, tc.session_id.get(), tc.player_id.get());
        ASSERT_TRUE(buffer.has_value());

        auto raw = deserializeBufferToMessage(std::move(*buffer));
        ASSERT_TRUE(raw.has_value());

        ASSERT_TRUE(isNetworkMessage(raw->type));
        auto netMsg = asNetworkMessage(raw->type);
        ASSERT_TRUE(netMsg.has_value());
        EXPECT_EQ(*netMsg, kTestMsgType);

        ASSERT_EQ(raw->args.size(), 2);
        EXPECT_EQ(raw->args[0], tc.session_id.get());
        EXPECT_EQ(raw->args[1], tc.player_id.get());

        auto event = makeEvent(raw->type, std::move(raw->args));
        ASSERT_NE(event, nullptr);
        auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
        ASSERT_NE(auth_event, nullptr);
        EXPECT_SESSION_EQ(auth_event->session_id.get(), tc.session_id.get());
        EXPECT_PLAYER_EQ(auth_event->player_id.get(), tc.player_id.get());
    }
}

TEST(IntegrationTest, SerializeDeserializeMultipleTimes) {
    SessionId session_id{123};
    PlayerId player_id{456};

    auto buffer1 = serializeMessageToBuffer(kTestMsgType, session_id.get(), player_id.get());
    ASSERT_TRUE(buffer1.has_value());

    auto raw1 = deserializeBufferToMessage(std::move(*buffer1));
    ASSERT_TRUE(raw1.has_value());

    auto buffer2 = serializeMessageToBuffer(raw1->type, raw1->args[0], raw1->args[1]);
    ASSERT_TRUE(buffer2.has_value());

    auto raw2 = deserializeBufferToMessage(std::move(*buffer2));
    ASSERT_TRUE(raw2.has_value());
    EXPECT_EQ(raw1->type, raw2->type);
    EXPECT_EQ(raw1->args, raw2->args);
}

// ============================================================================
// Performance tests (using public API)
// ============================================================================

TEST(PerformanceTest, SerializeDeserialize1000Messages) {
    const int NUM_MESSAGES = 1000;

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        auto buffer = serializeMessageToBuffer(kTestMsgType,
                                               SessionId{static_cast<uint64_t>(i)}.get(),
                                               PlayerId{static_cast<uint64_t>(i * 2)}.get());
        ASSERT_TRUE(buffer.has_value());

        auto raw = deserializeBufferToMessage(std::move(*buffer));
        ASSERT_TRUE(raw.has_value());

        ASSERT_TRUE(isNetworkMessage(raw->type));
        auto netMsg = asNetworkMessage(raw->type);
        ASSERT_TRUE(netMsg.has_value());
        EXPECT_EQ(*netMsg, kTestMsgType);

        ASSERT_EQ(raw->args.size(), 2);
        EXPECT_EQ(raw->args[0], i);
        EXPECT_EQ(raw->args[1], i * 2);

        auto event = makeEvent(raw->type, std::move(raw->args));
        ASSERT_NE(event, nullptr);
        auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
        ASSERT_NE(auth_event, nullptr);
        EXPECT_SESSION_EQ(auth_event->session_id.get(), i);
        EXPECT_PLAYER_EQ(auth_event->player_id.get(), i * 2);
    }
}

// ============================================================================
// Memory tests (no leaks)
// ============================================================================

TEST(MemoryTest, NoMemoryLeaksOnSerialization) {
    for (int i = 0; i < 100; ++i) {
        auto buffer = serializeMessageToBuffer(kTestMsgType, SessionId{123}.get(), PlayerId{456}.get());
        ASSERT_TRUE(buffer.has_value());

        auto raw = deserializeBufferToMessage(std::move(*buffer));
        EXPECT_TRUE(raw.has_value());
    }
}

TEST(MemoryTest, NoMemoryLeaksOnFailedDeserialization) {
    std::vector<uint8_t> invalid_data = {0xFF, 0xFF, 0xFF};
    auto raw = deserializeBufferToMessage(ByteBuffer{std::move(invalid_data)});
    EXPECT_FALSE(raw.has_value());
}
