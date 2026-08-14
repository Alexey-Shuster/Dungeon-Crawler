#include <common/game_state_dto.h>
#include <common/serialization_game_state.h>
#include <gtest/gtest.h>

using namespace network;

// -----------------------------------------------------------------------------
// Tests for BarrierSnapshot
// -----------------------------------------------------------------------------

TEST(SerializationGameStateTest, BarrierSnapshotRoundTrip) {
    BarrierSnapshot original{10, 20};
    auto cbor = serialize(original);
    ASSERT_NE(cbor, nullptr);

    auto deserialized = deserialize(cbor.get(), static_cast<BarrierSnapshot*>(nullptr));
    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(original, *deserialized);
}

TEST(SerializationGameStateTest, BarrierSnapshotDeserializeInvalid) {
    // Not an array => nullopt
    auto item = CborPtr(cbor_build_uint64(42));
    auto result = deserialize(item.get(), static_cast<BarrierSnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());

    // Array with wrong size (1 instead of 2)
    auto arr = CborPtr(cbor_new_definite_array(1));
    auto val = CborPtr(cbor_build_uint64(5));
    cbor_array_push(arr.get(), val.get());
    result = deserialize(arr.get(), static_cast<BarrierSnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());
}

// -----------------------------------------------------------------------------
// Tests for EntitySnapshot
// -----------------------------------------------------------------------------

TEST(SerializationGameStateTest, EntitySnapshotRoundTrip) {
    EntitySnapshot original{.type = 1, .state = 2, .id = 12345, .pos_x = 100, .pos_y = 200, .hp = 50};
    auto cbor = serialize(original);
    ASSERT_NE(cbor, nullptr);

    auto deserialized = deserialize(cbor.get(), static_cast<EntitySnapshot*>(nullptr));
    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(original, *deserialized);
}

TEST(SerializationGameStateTest, EntitySnapshotDeserializeInvalid) {
    // Not an array
    auto item = CborPtr(cbor_build_uint64(42));
    auto result = deserialize(item.get(), static_cast<EntitySnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());

    // Array with wrong size (5 instead of 6)
    auto arr = CborPtr(cbor_new_definite_array(5));
    for (int i = 0; i < 5; ++i) {
        auto val = CborPtr(cbor_build_uint64(i));
        cbor_array_push(arr.get(), val.get());
    }
    result = deserialize(arr.get(), static_cast<EntitySnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());
}

// -----------------------------------------------------------------------------
// Tests for GameMapSnapshot
// -----------------------------------------------------------------------------

TEST(SerializationGameStateTest, GameMapSnapshotRoundTrip) {
    GameMapSnapshot original;
    original.blc_x = 0;
    original.blc_y = 0;
    original.trc_x = 10;
    original.trc_y = 10;
    original.barriers = {{2, 3}, {5, 6}, {8, 9}};

    auto cbor = serialize(original);
    ASSERT_NE(cbor, nullptr);

    auto deserialized = deserialize(cbor.get(), static_cast<GameMapSnapshot*>(nullptr));
    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(original, *deserialized);
}

TEST(SerializationGameStateTest, GameMapSnapshotEmptyBarriers) {
    GameMapSnapshot original;
    original.blc_x = 0;
    original.blc_y = 0;
    original.trc_x = 5;
    original.trc_y = 5;
    // barriers empty

    auto cbor = serialize(original);
    ASSERT_NE(cbor, nullptr);

    auto deserialized = deserialize(cbor.get(), static_cast<GameMapSnapshot*>(nullptr));
    ASSERT_TRUE(deserialized.has_value());
    EXPECT_TRUE(deserialized->barriers.empty());
    EXPECT_EQ(original.blc_x, deserialized->blc_x);
    // etc.
}

TEST(SerializationGameStateTest, GameMapSnapshotDeserializeInvalid) {
    // Not an array
    auto item = CborPtr(cbor_build_uint64(42));
    auto result = deserialize(item.get(), static_cast<GameMapSnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());

    // Array with wrong size (4 instead of 5)
    auto arr = CborPtr(cbor_new_definite_array(4));
    for (int i = 0; i < 4; ++i) {
        auto val = CborPtr(cbor_build_uint64(i));
        cbor_array_push(arr.get(), val.get());
    }
    result = deserialize(arr.get(), static_cast<GameMapSnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());

    // 5th element is not an array (barriers)
    auto arr2 = CborPtr(cbor_new_definite_array(5));
    for (int i = 0; i < 4; ++i) {
        auto val = CborPtr(cbor_build_uint64(i));
        cbor_array_push(arr2.get(), val.get());
    }
    auto bad = CborPtr(cbor_build_uint64(99));  // not an array
    cbor_array_push(arr2.get(), bad.get());
    result = deserialize(arr2.get(), static_cast<GameMapSnapshot*>(nullptr));
    EXPECT_FALSE(result.has_value());
}

// -----------------------------------------------------------------------------
// Tests for full DungeonSnapshot (serializeGameState / deserializeGameState)
// -----------------------------------------------------------------------------

TEST(SerializationGameStateTest, DungeonSnapshotRoundTrip) {
    DungeonSnapshot original;
    original.game_map.blc_x = 0;
    original.game_map.blc_y = 0;
    original.game_map.trc_x = 20;
    original.game_map.trc_y = 20;
    original.game_map.barriers = {{1, 1}, {2, 2}, {3, 3}};

    EntitySnapshot e1{1, 1, 100, 10, 10, 30};
    EntitySnapshot e2{2, 0, 200, 15, 15, 0};  // dead monster
    original.entities = {e1, e2};

    auto msg = serializeGameState(original);
    ASSERT_TRUE(msg.has_value());

    auto deserialized = deserializeGameState(*msg);
    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(original, *deserialized);
}

TEST(SerializationGameStateTest, DungeonSnapshotEmptyEntities) {
    DungeonSnapshot original;
    original.game_map.blc_x = 0;
    original.game_map.blc_y = 0;
    original.game_map.trc_x = 10;
    original.game_map.trc_y = 10;
    // no barriers, no entities

    auto msg = serializeGameState(original);
    ASSERT_TRUE(msg.has_value());

    auto deserialized = deserializeGameState(*msg);
    ASSERT_TRUE(deserialized.has_value());
    EXPECT_TRUE(deserialized->game_map.barriers.empty());
    EXPECT_TRUE(deserialized->entities.empty());
}

TEST(SerializationGameStateTest, DeserializeGameStateInvalid) {
    // 1. Not a map (array)
    auto arr_msg = []() {
        auto arr = CborPtr(cbor_new_definite_array(1));
        auto val = CborPtr(cbor_build_uint64(42));
        cbor_array_push(arr.get(), val.get());
        auto msg_opt = cborToMessage(std::move(arr));
        return msg_opt.value();
    }();
    auto result = deserializeGameState(arr_msg);
    EXPECT_FALSE(result.has_value());

    // 2. Map missing "map" key
    auto bad_map_msg = []() {
        auto map = CborPtr(cbor_new_definite_map(1));
        auto key = CborPtr(cbor_build_string("wrong"));
        auto val = CborPtr(cbor_build_uint64(123));
        cbor_map_add(map.get(), {.key = key.get(), .value = val.get()});
        return cborToMessage(std::move(map)).value();
    }();
    result = deserializeGameState(bad_map_msg);
    EXPECT_FALSE(result.has_value());

    // 3. Map with "map" key but value is not an array
    auto bad_map_msg2 = []() {
        auto map = CborPtr(cbor_new_definite_map(1));
        auto key = CborPtr(cbor_build_string(kMap));
        auto val = CborPtr(cbor_build_uint64(999));  // not array
        cbor_map_add(map.get(), {.key = key.get(), .value = val.get()});
        return cborToMessage(std::move(map)).value();
    }();
    result = deserializeGameState(bad_map_msg2);
    EXPECT_FALSE(result.has_value());
}

// -----------------------------------------------------------------------------
// Tests for cborToMessage / messageToCbor (helpers)
// -----------------------------------------------------------------------------

TEST(SerializationGameStateTest, CborToMessageRoundTrip) {
    auto item = CborPtr(cbor_build_uint64(123456));
    auto msg_opt = cborToMessage(std::move(item));
    ASSERT_TRUE(msg_opt.has_value());

    cbor_load_result result;
    auto loaded = bufferToCbor(*msg_opt, &result);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(result.error.code, CBOR_ERR_NONE);
    EXPECT_TRUE(cbor_isa_uint(loaded.get()));
    EXPECT_EQ(cbor_get_uint64(loaded.get()), 123456);
}

TEST(SerializationGameStateTest, MessageToCborInvalid) {
    ByteBuffer invalid{ByteBuffer{0x01, 0x02, 0x03}};
    auto loaded = bufferToCbor(invalid);
    EXPECT_EQ(loaded, nullptr);
}

TEST(SerializationGameStateTest, MessageToCborTrulyInvalid) {
    ByteBuffer invalid{ByteBuffer{0xFF, 0xFF}};  // not a valid CBOR item
    auto loaded = bufferToCbor(invalid);
    EXPECT_EQ(loaded, nullptr);
}
