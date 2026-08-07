#include <cbor.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "events.h"
#include "message.h"
#include "message_dispatcher.h"
#include "serialization.h"

namespace serialization_test {

using namespace network;

// ============================================================================
// Вспомогательные функции
// ============================================================================

Message createMessage(const std::vector<uint8_t>& data) {
    MessageData msg_data = data;
    return Message{std::move(msg_data)};
}

struct CborTestDeleter {
    void operator()(cbor_item_t* item) const {
        if (item) {
            cbor_decref(&item);
        }
    }
};
using CborTestPtr = std::unique_ptr<cbor_item_t, CborTestDeleter>;

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

#define EXPECT_SESSION_EQ(actual, expected) EXPECT_EQ(static_cast<uint64_t>(actual), static_cast<uint64_t>(expected))
#define EXPECT_PLAYER_EQ(actual, expected) EXPECT_EQ(static_cast<uint64_t>(actual), static_cast<uint64_t>(expected))

// ============================================================================
// Тесты для addArgToArray (без изменений)
// ============================================================================

class AddArgToArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        array_.reset(cbor_new_definite_array(1));
        ASSERT_NE(array_, nullptr);
    }

    CborTestPtr array_;
};

TEST_F(AddArgToArrayTest, AddUint64Value) {
    EXPECT_TRUE(serialization::addArgToArray(array_.get(), 12345));

    EXPECT_EQ(cbor_array_size(array_.get()), 1);

    CborTestPtr item(cbor_array_get(array_.get(), 0));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_uint(item.get()));
    EXPECT_EQ(cbor_get_uint64(item.get()), 12345);
}

TEST_F(AddArgToArrayTest, AddZeroValue) {
    EXPECT_TRUE(serialization::addArgToArray(array_.get(), 0));

    CborTestPtr item(cbor_array_get(array_.get(), 0));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_uint(item.get()));
    EXPECT_EQ(cbor_get_uint64(item.get()), 0);
}

TEST_F(AddArgToArrayTest, AddMaxUint64) {
    EXPECT_TRUE(serialization::addArgToArray(array_.get(), UINT64_MAX));

    CborTestPtr item(cbor_array_get(array_.get(), 0));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_uint(item.get()));
    EXPECT_EQ(cbor_get_uint64(item.get()), UINT64_MAX);
}

TEST_F(AddArgToArrayTest, MultipleValues) {
    array_.reset(cbor_new_definite_array(3));
    ASSERT_NE(array_, nullptr);

    EXPECT_TRUE(serialization::addArgToArray(array_.get(), 100));
    EXPECT_TRUE(serialization::addArgToArray(array_.get(), 200));
    EXPECT_TRUE(serialization::addArgToArray(array_.get(), 300));

    EXPECT_EQ(cbor_array_size(array_.get()), 3);

    for (size_t i = 0; i < 3; ++i) {
        CborTestPtr item(cbor_array_get(array_.get(), i));
        ASSERT_NE(item, nullptr);
        EXPECT_TRUE(cbor_isa_uint(item.get()));
        EXPECT_EQ(cbor_get_uint64(item.get()), (i + 1) * 100);
    }
}

// ============================================================================
// Тесты для getMsgArgsFromCborArray (без изменений)
// ============================================================================

class GetMsgArgsFromCborArrayTest : public ::testing::Test {
protected:
    // Создаёт массив, где первый элемент — тип (uint8_t, но мы храним его как uint16_t?
    // В наших новых тестах мы используем uint16_t, но в старых тестах мы использовали uint8_t.
    // Однако getMsgArgsFromCborArray не смотрит на тип, он просто пропускает первый элемент.
    // Поэтому мы можем оставить создание с uint8_t для совместимости.
    CborTestPtr createDefiniteArray(uint16_t msg_type, const std::vector<uint64_t>& args = {}) {
        size_t total_size = 1 + args.size();
        CborTestPtr array(cbor_new_definite_array(total_size));
        if (!array) {
            return nullptr;
        }

        // Используем cbor_build_uint16, потому что теперь тип хранится как uint16_t
        CborTestPtr type_item(cbor_build_uint16(msg_type));
        if (!type_item) {
            return nullptr;
        }
        cbor_incref(type_item.get());
        cbor_array_set(array.get(), 0, type_item.get());

        for (size_t i = 0; i < args.size(); ++i) {
            CborTestPtr arg_item(cbor_build_uint64(args[i]));
            if (!arg_item) {
                return nullptr;
            }
            cbor_incref(arg_item.get());
            cbor_array_set(array.get(), 1 + i, arg_item.get());
        }

        return array;
    }
};

TEST_F(GetMsgArgsFromCborArrayTest, ArrayWithOnlyType) {
    auto array = createDefiniteArray(0);
    ASSERT_NE(array, nullptr);

    auto args = serialization::getMsgArgsFromCborArray(array.get());

    EXPECT_TRUE(args.has_value());
    EXPECT_TRUE(args->empty());
}

TEST_F(GetMsgArgsFromCborArrayTest, ValidArray) {
    auto array = createDefiniteArray(0, {123, 456});
    ASSERT_NE(array, nullptr);

    auto args = serialization::getMsgArgsFromCborArray(array.get());

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->size(), 2);
    EXPECT_EQ((*args)[0], 123);
    EXPECT_EQ((*args)[1], 456);
}

TEST_F(GetMsgArgsFromCborArrayTest, LargeUint64Values) {
    auto array = createDefiniteArray(0, {UINT64_MAX, 0x0123456789ABCDEF});
    ASSERT_NE(array, nullptr);

    auto args = serialization::getMsgArgsFromCborArray(array.get());

    ASSERT_TRUE(args.has_value());
    EXPECT_EQ(args->size(), 2);
    EXPECT_EQ((*args)[0], UINT64_MAX);
    EXPECT_EQ((*args)[1], 0x0123456789ABCDEF);
}

TEST_F(GetMsgArgsFromCborArrayTest, NullArrayReturnsNullopt) {
    auto args = serialization::getMsgArgsFromCborArray(nullptr);
    EXPECT_FALSE(args.has_value());
}

TEST_F(GetMsgArgsFromCborArrayTest, NonArrayReturnsNullopt) {
    CborTestPtr item(cbor_build_uint8(42));
    auto args = serialization::getMsgArgsFromCborArray(item.get());
    EXPECT_FALSE(args.has_value());
}

TEST_F(GetMsgArgsFromCborArrayTest, EmptyCborArrayReturnsNullopt) {
    CborTestPtr empty_array(cbor_new_definite_array(0));
    ASSERT_NE(empty_array, nullptr);

    auto args = serialization::getMsgArgsFromCborArray(empty_array.get());
    EXPECT_FALSE(args.has_value());
}

// ============================================================================
// Тесты для makeEvent (теперь в message_dispatcher)
// ============================================================================

class MakeEventTest : public ::testing::Test {
protected:
    using MessageArgs = serialization::MessageArgs;  // теперь из serialization.h
};

TEST_F(MakeEventTest, CreateAuthRequestedEvent) {
    MessageArgs args = std::vector<uint64_t>{123, 456};

    auto event = message_dispatcher::makeEvent(message::NetworkMessageType::kJoin, args);

    ASSERT_NE(event, nullptr);
    auto auth_event = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);
    EXPECT_SESSION_EQ(auth_event->session_id.value, 123);
    EXPECT_PLAYER_EQ(auth_event->player_id.value, 456);
}

TEST_F(MakeEventTest, UnknownMessageTypeReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{123, 456};

    auto event = message_dispatcher::makeEvent(std::monostate{}, args);

    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, InvalidArgsCountReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{123};

    auto event = message_dispatcher::makeEvent(message::NetworkMessageType::kJoin, args);

    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, EmptyArgsReturnsNull) {
    MessageArgs args = std::vector<uint64_t>{};

    auto event = message_dispatcher::makeEvent(message::NetworkMessageType::kJoin, args);

    EXPECT_EQ(event, nullptr);
}

TEST_F(MakeEventTest, NulloptArgsReturnsNull) {
    MessageArgs args = std::nullopt;

    auto event = message_dispatcher::makeEvent(message::NetworkMessageType::kJoin, args);

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

    auto msg_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin, session_id, player_id);
    ASSERT_TRUE(msg_opt.has_value());

    auto& msg = msg_opt.value();
    EXPECT_FALSE(msg.message_data.empty());

    auto event = message_dispatcher::deserializeMessage(msg);
    ASSERT_NE(event, nullptr);

    auto auth_event = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);

    EXPECT_SESSION_EQ(auth_event->session_id.value, 123);
    EXPECT_PLAYER_EQ(auth_event->player_id.value, 456);
}

TEST_F(SerializationIntegrationTest, SerializeDeserializeWithLargeIds) {
    SessionId session_id{0xFFFFFFFFFFFFFFFF};
    PlayerId player_id{0x0123456789ABCDEF};

    auto msg_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin, session_id, player_id);
    ASSERT_TRUE(msg_opt.has_value());

    auto event = message_dispatcher::deserializeMessage(msg_opt.value());
    ASSERT_NE(event, nullptr);

    auto auth_event = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event);
    ASSERT_NE(auth_event, nullptr);

    EXPECT_SESSION_EQ(auth_event->session_id.value, 0xFFFFFFFFFFFFFFFF);
    EXPECT_PLAYER_EQ(auth_event->player_id.value, 0x0123456789ABCDEF);
}

TEST_F(SerializationIntegrationTest, SerializeMessageCreatesCorrectFormat) {
    auto msg_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin, SessionId{123}, PlayerId{456});
    ASSERT_TRUE(msg_opt.has_value());

    auto& msg = msg_opt.value();
    EXPECT_FALSE(msg.message_data.empty());

    cbor_load_result load_result;
    CborTestPtr item(cbor_load(msg.message_data.data(), msg.message_data.size(), &load_result));

    EXPECT_EQ(load_result.error.code, CBOR_ERR_NONE);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(cbor_isa_array(item.get()));
    // Массив должен содержать: тип (uint16_t) + 2 аргумента = 3 элемента
    EXPECT_EQ(cbor_array_size(item.get()), 3);
}

TEST_F(SerializationIntegrationTest, SerializeUnknownMessageTypeReturnsNullopt) {
    auto msg_opt = serialization::serializeMessage(std::monostate{}, SessionId{123}, PlayerId{456});
    EXPECT_FALSE(msg_opt.has_value());
}

TEST_F(SerializationIntegrationTest, DeserializeInvalidDataReturnsNull) {
    std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02};
    Message msg = createMessage(invalid_data);

    auto event = message_dispatcher::deserializeMessage(msg);
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeEmptyMessageReturnsNull) {
    Message msg = createMessage({});

    auto event = message_dispatcher::deserializeMessage(msg);
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeNonArrayReturnsNull) {
    CborTestPtr item(cbor_build_uint8(42));
    auto data = serializeCbor(item.get());
    ASSERT_FALSE(data.empty());

    Message msg = createMessage(data);

    auto event = message_dispatcher::deserializeMessage(msg);
    EXPECT_EQ(event, nullptr);
}

TEST_F(SerializationIntegrationTest, DeserializeEmptyArrayReturnsNull) {
    CborTestPtr array(cbor_new_definite_array(0));
    ASSERT_NE(array, nullptr);

    auto data = serializeCbor(array.get());
    ASSERT_FALSE(data.empty());

    Message msg = createMessage(data);

    auto event = message_dispatcher::deserializeMessage(msg);
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

    Message msg = createMessage(data);

    auto event = message_dispatcher::deserializeMessage(msg);
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

    Message msg = createMessage(data);

    auto event = message_dispatcher::deserializeMessage(msg);
    EXPECT_EQ(event, nullptr);
}

// ============================================================================
// Тесты для deserializeMessageRaw (новая функция)
// ============================================================================

TEST(DeserializeRawTest, ValidMessage) {
    auto msg_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin, SessionId{123}, PlayerId{456});
    ASSERT_TRUE(msg_opt.has_value());

    auto raw = serialization::deserializeMessageRaw(msg_opt.value());
    ASSERT_TRUE(raw.has_value());
    ASSERT_TRUE(std::holds_alternative<message::NetworkMessageType>(raw->type));
    EXPECT_EQ(std::get<message::NetworkMessageType>(raw->type), message::NetworkMessageType::kJoin);
    ASSERT_EQ(raw->args.size(), 2);
    EXPECT_EQ(raw->args[0], 123);
    EXPECT_EQ(raw->args[1], 456);
}

TEST(DeserializeRawTest, InvalidDataReturnsNullopt) {
    std::vector<uint8_t> invalid = {0xFF, 0xFF};
    Message msg = createMessage(invalid);
    auto raw = serialization::deserializeMessageRaw(msg);
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
        auto msg_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin, tc.session_id, tc.player_id);
        ASSERT_TRUE(msg_opt.has_value());

        auto event = message_dispatcher::deserializeMessage(msg_opt.value());
        ASSERT_NE(event, nullptr);

        auto auth_event = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event);
        ASSERT_NE(auth_event, nullptr);

        EXPECT_SESSION_EQ(auth_event->session_id.value, tc.session_id.value);
        EXPECT_PLAYER_EQ(auth_event->player_id.value, tc.player_id.value);
    }
}

TEST(IntegrationTest, SerializeDeserializeMultipleTimes) {
    SessionId session_id{123};
    PlayerId player_id{456};

    auto msg1_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin, session_id, player_id);
    ASSERT_TRUE(msg1_opt.has_value());

    auto event1 = message_dispatcher::deserializeMessage(msg1_opt.value());
    ASSERT_NE(event1, nullptr);

    auto auth_event1 = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event1);
    ASSERT_NE(auth_event1, nullptr);

    auto msg2_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin,
                                                    SessionId{auth_event1->session_id},
                                                    PlayerId{auth_event1->player_id});
    ASSERT_TRUE(msg2_opt.has_value());

    auto event2 = message_dispatcher::deserializeMessage(msg2_opt.value());
    ASSERT_NE(event2, nullptr);

    auto auth_event2 = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event2);
    ASSERT_NE(auth_event2, nullptr);

    EXPECT_SESSION_EQ(auth_event1->session_id.value, auth_event2->session_id.value);
    EXPECT_PLAYER_EQ(auth_event1->player_id.value, auth_event2->player_id.value);
}

// ============================================================================
// Тесты производительности
// ============================================================================

TEST(PerformanceTest, SerializeDeserialize1000Messages) {
    const int NUM_MESSAGES = 1000;

    for (int i = 0; i < NUM_MESSAGES; ++i) {
        auto msg_opt = serialization::serializeMessage(message::NetworkMessageType::kJoin,
                                                       SessionId{static_cast<uint64_t>(i)},
                                                       PlayerId{static_cast<uint64_t>(i * 2)});
        ASSERT_TRUE(msg_opt.has_value());

        auto event = message_dispatcher::deserializeMessage(msg_opt.value());
        ASSERT_NE(event, nullptr);

        auto auth_event = std::dynamic_pointer_cast<events::AuthRequestedEvent>(event);
        ASSERT_NE(auth_event, nullptr);

        EXPECT_SESSION_EQ(auth_event->session_id.value, i);
        EXPECT_PLAYER_EQ(auth_event->player_id.value, i * 2);
    }
}

// ============================================================================
// Тесты для утечек памяти
// ============================================================================

TEST(MemoryTest, NoMemoryLeaksOnSerialization) {
    for (int i = 0; i < 100; ++i) {
        auto msg_opt =
            serialization::serializeMessage(message::NetworkMessageType::kJoin, SessionId{123}, PlayerId{456});
        ASSERT_TRUE(msg_opt.has_value());

        auto event = message_dispatcher::deserializeMessage(msg_opt.value());
        EXPECT_NE(event, nullptr);
    }
}

TEST(MemoryTest, NoMemoryLeaksOnFailedDeserialization) {
    std::vector<uint8_t> invalid_data = {0xFF, 0xFF, 0xFF};
    Message msg = createMessage(invalid_data);

    auto event = message_dispatcher::deserializeMessage(msg);
    EXPECT_EQ(event, nullptr);
}

}  // namespace serialization_test
