#include <common/network/game_state_dto.h>
#include <common/wire/serder_game_state.h>
#include <cstdint>
#include <gtest/gtest.h>

using namespace dungeons::common::network;
using namespace dungeons::common::wire;

// Helper to create a sample DungeonSnapshot for testing
static DungeonSnapshot makeSampleSnapshot() {
    DungeonSnapshot snapshot;

    // Map
    snapshot.game_map.blc_x = 0;
    snapshot.game_map.blc_y = 0;
    snapshot.game_map.trc_x = 20;
    snapshot.game_map.trc_y = 20;
    snapshot.game_map.barriers = {{1, 1}, {2, 2}, {3, 3}};

    // Players
    EntitySnapshot player1{1, 1, 100, 10, 10, 30};  // alive
    EntitySnapshot player2{1, 0, 101, 12, 12, 0};   // dead
    snapshot.players = {player1, player2};

    // Mobs
    EntitySnapshot mob1{2, 1, 200, 15, 15, 40};  // alive
    snapshot.mobs = {mob1};

    return snapshot;
}

// ============================================================================
// Round‑trip tests
// ============================================================================

TEST(SerializationGameStateTest, FullRoundTrip) {
    auto original = makeSampleSnapshot();

    auto buffer = serializeGameState(original);
    ASSERT_TRUE(buffer.has_value());

    auto deserialized = deserializeGameState(*buffer);
    ASSERT_TRUE(deserialized.has_value());

    EXPECT_EQ(original, *deserialized);
}

TEST(SerializationGameStateTest, RoundTripWithEmptyCollections) {
    DungeonSnapshot original;
    original.game_map.blc_x = 0;
    original.game_map.blc_y = 0;
    original.game_map.trc_x = 10;
    original.game_map.trc_y = 10;
    // barriers, players, mobs left empty

    auto buffer = serializeGameState(original);
    ASSERT_TRUE(buffer.has_value());

    auto deserialized = deserializeGameState(*buffer);
    ASSERT_TRUE(deserialized.has_value());

    EXPECT_TRUE(deserialized->game_map.barriers.empty());
    EXPECT_TRUE(deserialized->players.empty());
    EXPECT_TRUE(deserialized->mobs.empty());
}

TEST(SerializationGameStateTest, RoundTripWithLargeValues) {
    DungeonSnapshot original;
    original.game_map.blc_x = -1000;
    original.game_map.blc_y = -1000;
    original.game_map.trc_x = 1000;
    original.game_map.trc_y = 1000;

    original.game_map.barriers = {BarrierSnapshot{UINT64_MAX, UINT64_MAX}, BarrierSnapshot{0, 0}};

    EntitySnapshot player{1, 1, UINT64_MAX, 123456789, 987654321, 100};
    original.players = {player};

    EntitySnapshot mob{2, 0, UINT64_MAX - 1, 111, 222, 0};
    original.mobs = {mob};

    auto buffer = serializeGameState(original);
    ASSERT_TRUE(buffer.has_value());

    auto deserialized = deserializeGameState(*buffer);
    ASSERT_TRUE(deserialized.has_value());

    // Compare field by field
    EXPECT_EQ(original.game_map.blc_x, deserialized->game_map.blc_x);
    EXPECT_EQ(original.game_map.blc_y, deserialized->game_map.blc_y);
    EXPECT_EQ(original.game_map.trc_x, deserialized->game_map.trc_x);
    EXPECT_EQ(original.game_map.trc_y, deserialized->game_map.trc_y);
    EXPECT_EQ(original.game_map.barriers, deserialized->game_map.barriers);
    EXPECT_EQ(original.players, deserialized->players);
    EXPECT_EQ(original.mobs, deserialized->mobs);
}

// ============================================================================
// Deserialization error cases (public API)
// ============================================================================

TEST(SerializationGameStateTest, DeserializeEmptyBuffer) {
    ByteBuffer empty;
    auto result = deserializeGameState(empty);
    EXPECT_FALSE(result.has_value());
}

TEST(SerializationGameStateTest, DeserializeInvalidCbor) {
    // A single byte that is not a valid CBOR map
    ByteBuffer invalid = {0x01};
    auto result = deserializeGameState(invalid);
    EXPECT_FALSE(result.has_value());
}

TEST(SerializationGameStateTest, DeserializeNonMapCbor) {
    // CBOR array (not a map)
    ByteBuffer array = {0x80};  // empty array
    auto result = deserializeGameState(array);
    EXPECT_FALSE(result.has_value());
}

TEST(SerializationGameStateTest, DeserializeMapMissingKeys) {
    // CBOR map with only "map" key (missing "players" and "mobs")
    // This is a minimal but incomplete snapshot.
    std::vector<uint8_t> data = {
        0xA1,  // map of 1 pair
        0x63,
        0x6D,
        0x61,
        0x70,  // "map"
        0x85,  // array of 5
        0x00,
        0x00,
        0x0A,
        0x0A,  // blc_x=0, blc_y=0, trc_x=10, trc_y=10
        0x80   // empty barriers array
    };
    ByteBuffer buffer(data);
    auto result = deserializeGameState(buffer);
    EXPECT_FALSE(result.has_value());  // missing players/mobs
}

TEST(SerializationGameStateTest, DeserializeMapWithWrongValueTypes) {
    // Map with "map" key as integer (should be array)
    std::vector<uint8_t> data = {
        0xA1,  // map of 1 pair
        0x63,
        0x6D,
        0x61,
        0x70,  // "map"
        0x18,
        0x2A  // integer 42 (not array)
    };
    ByteBuffer buffer(data);
    auto result = deserializeGameState(buffer);
    EXPECT_FALSE(result.has_value());
}

TEST(SerializationGameStateTest, DeserializeMapWithInvalidBarriers) {
    // "map" array with barriers element as integer instead of array
    std::vector<uint8_t> data = {
        0xA1,  // map of 1 pair
        0x63,
        0x6D,
        0x61,
        0x70,  // "map"
        0x85,  // array of 5
        0x00,
        0x00,
        0x0A,
        0x0A,  // blc_x=0, blc_y=0, trc_x=10, trc_y=10
        0x18,
        0x2A  // 42 (not array)
    };
    ByteBuffer buffer(data);
    auto result = deserializeGameState(buffer);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Additional sanity: serialization produces something
// ============================================================================

TEST(SerializationGameStateTest, SerializationProducesNonEmptyBuffer) {
    auto snapshot = makeSampleSnapshot();
    auto buffer = serializeGameState(snapshot);
    ASSERT_TRUE(buffer.has_value());
    EXPECT_FALSE(buffer->empty());
}
