#include <common/network/byte_buffer.h>
#include <common/types/message_types.h>
#include <common/wire/serder.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

using namespace dungeons::common::network;
using namespace dungeons::common::wire;
using namespace dungeons::common::types;

// Helper to create a valid MessageTypeVariant for testing.
// Replace with actual type creation logic from your project.
static dungeons::dc_MsgVariant makeTestMessageType(uint16_t id = 1) {
    return dungeons::dc_MsgVariant{NetworkMessageType::kJoin};
}

// ============================================================================
// Serialization / Deserialization Integration Tests
// ============================================================================

TEST(SerDerTest, SerializeAndDeserialize_NoArgs) {
    auto msg_type = makeTestMessageType();
    auto buffer = serializeMessageToBuffer(msg_type);
    ASSERT_TRUE(buffer.has_value());

    auto result = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, msg_type);
    EXPECT_TRUE(result->args.empty());
}

TEST(SerDerTest, SerializeAndDeserialize_WithArgs) {
    auto msg_type = makeTestMessageType();
    MessageArgs args = {123, 456, 789};

    auto buffer = serializeMessageToBuffer(msg_type, std::move(args));
    ASSERT_TRUE(buffer.has_value());

    auto result = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, msg_type);
    EXPECT_EQ(result->args, (MessageArgs{123, 456, 789}));
}

TEST(SerDerTest, SerializeAndDeserialize_WithLargeValues) {
    auto msg_type = makeTestMessageType();
    MessageArgs args = {0, UINT64_MAX, 0x0123456789ABCDEF};

    auto buffer = serializeMessageToBuffer(msg_type, std::move(args));
    ASSERT_TRUE(buffer.has_value());

    auto result = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, msg_type);
    EXPECT_EQ(result->args, (MessageArgs{0, UINT64_MAX, 0x0123456789ABCDEF}));
}

TEST(SerDerTest, Deserialize_EmptyBuffer_ReturnsNullopt) {
    ByteBuffer empty;
    auto result = deserializeBufferToMessage(std::move(empty));
    EXPECT_FALSE(result.has_value());
}

TEST(SerDerTest, Deserialize_InvalidCbor_ReturnsNullopt) {
    // A single byte that is not a valid CBOR array
    ByteBuffer invalid = {0x01};
    auto result = deserializeBufferToMessage(std::move(invalid));
    EXPECT_FALSE(result.has_value());
}

TEST(SerDerTest, Deserialize_EmptyArray_ReturnsNullopt) {
    // CBOR encoding of an empty array: 0x80
    ByteBuffer empty_array = {0x80};
    auto result = deserializeBufferToMessage(std::move(empty_array));
    EXPECT_FALSE(result.has_value());
}

TEST(SerDerTest, Deserialize_ArrayWithNonUintType_ReturnsNullopt) {
    // CBOR array: [null] (not a uint type)
    ByteBuffer invalid_type = {0x81, 0xF6};  // array of length 1, null
    auto result = deserializeBufferToMessage(std::move(invalid_type));
    EXPECT_FALSE(result.has_value());
}

TEST(SerDerTest, Deserialize_ArrayWithUintTypeButNoArgs) {
    // CBOR array: [1] (type id 1)
    ByteBuffer valid = {0x81, 0x01};  // definite array of length 1, uint 1
    auto result = deserializeBufferToMessage(std::move(valid));
    ASSERT_TRUE(result.has_value());
    // The type should be unpacked correctly; we can't compare directly without knowing the mapping,
    // but we can check that args is empty and type is not monostate.
    EXPECT_FALSE(std::holds_alternative<std::monostate>(result->type));
    EXPECT_TRUE(result->args.empty());
}

TEST(SerDerTest, Deserialize_ArrayWithArgs) {
    // CBOR array: [1, 10, 20] -> type=1, args=[10,20]
    // Encoding: 0x83 (array of 3), 0x01 (type), 0x0A (10), 0x14 (20)
    ByteBuffer data = {0x83, 0x01, 0x0A, 0x14};
    auto result = deserializeBufferToMessage(std::move(data));
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(std::holds_alternative<std::monostate>(result->type));
    EXPECT_EQ(result->args, (MessageArgs{10, 20}));
}

// Variadic template overload test
TEST(SerDerTest, VariadicTemplateOverload) {
    auto msg_type = makeTestMessageType();
    auto buffer = serializeMessageToBuffer(msg_type, 100, 200, 300);
    ASSERT_TRUE(buffer.has_value());

    auto result = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->args, (MessageArgs{100, 200, 300}));
}

// Test round-trip with move semantics
TEST(SerDerTest, SerializeWithRValueArgs) {
    auto msg_type = makeTestMessageType();
    MessageArgs args = {5, 10, 15};
    auto buffer = serializeMessageToBuffer(msg_type, std::move(args));
    ASSERT_TRUE(buffer.has_value());
    // args is now moved-from, but we don't use it further.
    auto result = deserializeBufferToMessage(std::move(*buffer));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->args, (MessageArgs{5, 10, 15}));
}
