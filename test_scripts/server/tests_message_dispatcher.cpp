#include <cbor.h>
#include <common/network/raw_message.h>
#include <common/wire/serder.h>
#include <common/wire/serder_base.h>
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

struct CborTestDeleter {
    void operator()(cbor_item_t* item) const {
        if (item) {
            cbor_decref(&item);
        }
    }
};

using CborTestPtr = std::unique_ptr<cbor_item_t, CborTestDeleter>;

RawMessage createMessage(std::vector<uint8_t> data) {
    return RawMessage{std::move(data)};
}

std::vector<uint8_t> serializeCbor(cbor_item_t* item) {
    uint8_t* buffer = nullptr;
    size_t buffer_size = 0;

    if (cbor_serialize_alloc(item, &buffer, &buffer_size) == 0) {
        free(buffer);
        return {};
    }

    std::vector<uint8_t> result(buffer, buffer + buffer_size);
    free(buffer);
    return result;
}

class MakeEventTest : public ::testing::Test {
protected:
    using MessageArgs = dungeons::common::wire::MessageArgs;
};

TEST_F(MakeEventTest, CreateAuthRequestedEvent) {
    MessageArgs args = std::vector<uint64_t>{123, 456};

    auto event = makeEvent(NetworkMessageType::kJoin, args);

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

    auto event = makeEvent(NetworkMessageType::kJoin, args);

    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, EmptyArgsReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{};

    auto event = makeEvent(NetworkMessageType::kJoin, args);

    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, NulloptArgsReturnsNull) {
    MessageArgs args = std::nullopt;

    auto event = makeEvent(NetworkMessageType::kJoin, args);

    EXPECT_EQ(event, nullptr);
}

// ============================================================================
// Интеграционные тесты для serializeMessage / deserializeMessage
// ============================================================================

class SerializationIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SerializationIntegrationTest, SerializeDeserializeJoinMessage) {
    SessionId session_id{123};
    PlayerId player_id{456};

    auto msg_opt = serializeMessage(NetworkMessageType::kJoin, session_id.get(), player_id.get());
    ASSERT_TRUE(msg_opt.has_value());

    auto& msg = msg_opt.value();
    EXPECT_FALSE(msg.empty());

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    ASSERT_NE(event, nullptr);

    auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);

    EXPECT_SESSION_EQ(auth_event->session_id.get(), 123);
    EXPECT_PLAYER_EQ(auth_event->player_id.get(), 456);
}

TEST_F(SerializationIntegrationTest, SerializeDeserializeWithLargeIds) {
    SessionId session_id{0xFFFFFFFFFFFFFFFF};
    PlayerId player_id{0x0123456789ABCDEF};

    auto msg_opt = serializeMessage(NetworkMessageType::kJoin, session_id.get(), player_id.get());
    ASSERT_TRUE(msg_opt.has_value());

    auto event = deserializeMessage(RawMessage{std::move(*msg_opt)});
    ASSERT_NE(event, nullptr);

    auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);

    EXPECT_SESSION_EQ(auth_event->session_id.get(), 0xFFFFFFFFFFFFFFFF);
    EXPECT_PLAYER_EQ(auth_event->player_id.get(), 0x0123456789ABCDEF);
}

TEST_F(SerializationIntegrationTest, SerializeMessageCreatesCorrectFormat) {
    auto msg_opt = serializeMessage(NetworkMessageType::kJoin, SessionId{123}.get(), PlayerId{456}.get());
    ASSERT_TRUE(msg_opt.has_value());

    auto& msg = msg_opt.value();
    EXPECT_FALSE(msg.empty());

    cbor_load_result load_result;
    CborTestPtr item(cbor_load(msg.data(), msg.size(), &load_result));

    EXPECT_EQ(load_result.error.code, CBOR_ERR_NONE);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_array(item.get()));
    // Массив должен содержать: тип (uint16_t) + 2 аргумента = 3 элемента
    EXPECT_EQ(cbor_array_size(item.get()), 3);
}

TEST_F(SerializationIntegrationTest, SerializeUnknownMessageTypeReturnsNullopt) {
    auto msg_opt = serializeMessage(std::monostate{}, SessionId{123}.get(), PlayerId{456}.get());
    EXPECT_FALSE(msg_opt.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeInvalidDataReturnsNull) {
    std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02};
    RawMessage msg{createMessage(invalid_data)};

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeEmptyMessageReturnsNull) {
    RawMessage msg{createMessage({})};

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeNonArrayReturnsNull) {
    CborTestPtr item(cbor_build_uint8(42));
    auto data = serializeCbor(item.get());
    ASSERT_FALSE(data.empty());

    RawMessage msg = createMessage(data);

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeEmptyArrayReturnsNull) {
    CborTestPtr array(cbor_new_definite_array(0));
    ASSERT_NE(array, nullptr);

    auto data = serializeCbor(array.get());
    ASSERT_FALSE(data.empty());

    RawMessage msg = createMessage(data);

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeInvalidMessageTypeReturnsNull) {
    // Создаём массив с невалидным типом (строка вместо uint16)
    CborTestPtr array(cbor_new_definite_array(2));
    ASSERT_NE(array, nullptr);

    CborTestPtr type_item(cbor_build_string("invalid"));
    ASSERT_NE(type_item, nullptr);
    cbor_incref(type_item.get());
    cbor_array_set(array.get(), 0, type_item.get());

    CborTestPtr arg_item(cbor_build_uint64(123));
    ASSERT_NE(arg_item, nullptr);
    cbor_incref(arg_item.get());
    cbor_array_set(array.get(), 1, arg_item.get());

    auto data = serializeCbor(array.get());
    ASSERT_FALSE(data.empty());

    RawMessage msg = createMessage(data);

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeWithInvalidArgsReturnsNull) {
    // Создаём массив с корректным типом (uint16), но невалидным аргументом (строка)
    CborTestPtr array(cbor_new_definite_array(2));
    ASSERT_NE(array, nullptr);

    CborTestPtr type_item(cbor_build_uint16(0));  // kJoin = 0
    ASSERT_NE(type_item, nullptr);
    cbor_incref(type_item.get());
    cbor_array_set(array.get(), 0, type_item.get());

    CborTestPtr invalid_arg(cbor_build_string("not a number"));
    ASSERT_NE(invalid_arg, nullptr);
    cbor_incref(invalid_arg.get());
    cbor_array_set(array.get(), 1, invalid_arg.get());

    auto data = serializeCbor(array.get());
    ASSERT_FALSE(data.empty());

    RawMessage msg = createMessage(data);

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}

// ============================================================================
// Тесты для deserializeMessageRaw
// ============================================================================

TEST(DeserializeRawTest, ValidMessage) {
    auto msg_opt = serializeMessage(NetworkMessageType::kJoin, SessionId{123}.get(), PlayerId{456}.get());
    ASSERT_TRUE(msg_opt.has_value());

    auto raw = deserializeRawMessage(*msg_opt);
    ASSERT_TRUE(raw.has_value());
    ASSERT_TRUE(std::holds_alternative<NetworkMessageType>(raw->type));
    EXPECT_EQ(std::get<NetworkMessageType>(raw->type), NetworkMessageType::kJoin);
    ASSERT_EQ(raw->args.size(), 2);
    EXPECT_EQ(raw->args[0], 123);
    EXPECT_EQ(raw->args[1], 456);
}

TEST(DeserializeRawTest, InvalidDataReturnsNullopt) {
    std::vector<uint8_t> invalid = {0xFF, 0xFF};
    RawMessage msg = createMessage(invalid);
    auto raw = deserializeRawMessage(msg.buffer);
    EXPECT_FALSE(raw.has_value());
}

// ============================================================================
// Интеграционные тесты с различными значениями
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
        auto msg_opt = serializeMessage(NetworkMessageType::kJoin, tc.session_id.get(), tc.player_id.get());
        ASSERT_TRUE(msg_opt.has_value());

        auto event = deserializeMessage(RawMessage{std::move(*msg_opt)});
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

    auto msg1_opt = serializeMessage(NetworkMessageType::kJoin, session_id.get(), player_id.get());
    ASSERT_TRUE(msg1_opt.has_value());

    auto event1 = deserializeMessage(RawMessage{std::move(*msg1_opt)});
    ASSERT_NE(event1, nullptr);

    auto auth_event1 = std::dynamic_pointer_cast<AuthRequestedEvent>(event1);
    ASSERT_NE(auth_event1, nullptr);

    auto msg2_opt = serializeMessage(NetworkMessageType::kJoin,
                                     SessionId{auth_event1->session_id}.get(),
                                     PlayerId{auth_event1->player_id}.get());
    ASSERT_TRUE(msg2_opt.has_value());

    auto event2 = deserializeMessage(RawMessage{std::move(*msg2_opt)});
    ASSERT_NE(event2, nullptr);

    auto auth_event2 = std::dynamic_pointer_cast<AuthRequestedEvent>(event2);
    ASSERT_NE(auth_event2, nullptr);

    EXPECT_SESSION_EQ(auth_event1->session_id.get(), auth_event2->session_id.get());
    EXPECT_PLAYER_EQ(auth_event1->player_id.get(), auth_event2->player_id.get());
}

// ============================================================================
// Тесты производительности
// ============================================================================

TEST(PerformanceTest, SerializeDeserialize1000Messages) {
    const int NUM_MESSAGES = 1000;

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        auto msg_opt = serializeMessage(NetworkMessageType::kJoin,
                                        SessionId{static_cast<uint64_t>(i)}.get(),
                                        PlayerId{static_cast<uint64_t>(i * 2)}.get());
        ASSERT_TRUE(msg_opt.has_value());

        auto event = deserializeMessage(RawMessage{std::move(*msg_opt)});
        ASSERT_NE(event, nullptr);

        auto auth_event = std::dynamic_pointer_cast<AuthRequestedEvent>(event);
        ASSERT_NE(auth_event, nullptr);

        EXPECT_SESSION_EQ(auth_event->session_id.get(), i);
        EXPECT_PLAYER_EQ(auth_event->player_id.get(), i * 2);
    }
}

// ============================================================================
// Тесты для утечек памяти
// ============================================================================

TEST(MemoryTest, NoMemoryLeaksOnSerialization) {
    for (int i = 0; i < 100; ++i) {
        auto msg_opt = serializeMessage(NetworkMessageType::kJoin, SessionId{123}.get(), PlayerId{456}.get());
        ASSERT_TRUE(msg_opt.has_value());

        auto event = deserializeMessage(RawMessage{std::move(*msg_opt)});
        EXPECT_NE(event, nullptr);
    }
}

TEST(MemoryTest, NoMemoryLeaksOnFailedDeserialization) {
    std::vector<uint8_t> invalid_data = {0xFF, 0xFF, 0xFF};
    RawMessage msg = createMessage(invalid_data);

    auto event = deserializeMessage(RawMessage{std::move(msg)});
    EXPECT_EQ(event, nullptr);
}
