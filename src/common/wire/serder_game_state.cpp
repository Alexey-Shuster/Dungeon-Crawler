#include "serder_game_state.h"

#include <format>
#include <utility/logger.h>

namespace dungeons::common::wire {

using namespace dungeons::common::network;

namespace {

inline constexpr size_t kSnapshotSize = 3;
inline constexpr size_t kBarrierArraySize = 2;
inline constexpr size_t kEntityArraySize = 6;
inline constexpr size_t kGameMapArraySize = 5;

}  // namespace

namespace detail {

CborPtr serialize(const BarrierSnapshot& barrier) {
    return buildArray(cbor_build_uint64(barrier.pos_x), cbor_build_uint64(barrier.pos_y));
}

std::optional<BarrierSnapshot> deserialize(cbor_item_t* item, BarrierSnapshot*) {
    if (!item || !cbor_isa_array(item) || cbor_array_size(item) != kBarrierArraySize)
        return std::nullopt;
    auto x = readCborUint(cbor_array_get(item, 0));
    auto y = readCborUint(cbor_array_get(item, 1));
    if (!x || !y)
        return std::nullopt;
    return BarrierSnapshot{.pos_x = static_cast<uint32_t>(*x), .pos_y = static_cast<uint32_t>(*y)};
}

CborPtr serialize(const EntitySnapshot& entity) {
    return buildArray(cbor_build_uint64(entity.type),
                      cbor_build_uint64(entity.state),
                      cbor_build_uint64(entity.id),
                      cbor_build_uint64(entity.pos_x),
                      cbor_build_uint64(entity.pos_y),
                      cbor_build_uint64(entity.hp));
}

std::optional<EntitySnapshot> deserialize(cbor_item_t* item, EntitySnapshot*) {
    if (!item || !cbor_isa_array(item) || cbor_array_size(item) != kEntityArraySize)
        return std::nullopt;
    auto type = readCborUint(cbor_array_get(item, 0));
    auto state = readCborUint(cbor_array_get(item, 1));
    auto id = readCborUint(cbor_array_get(item, 2));
    auto pos_x = readCborUint(cbor_array_get(item, 3));
    auto pos_y = readCborUint(cbor_array_get(item, 4));
    auto hp = readCborUint(cbor_array_get(item, 5));
    if (!type || !state || !id || !pos_x || !pos_y || !hp)
        return std::nullopt;
    return EntitySnapshot{.type = static_cast<uint8_t>(*type),
                          .state = static_cast<uint8_t>(*state),
                          .id = *id,
                          .pos_x = static_cast<uint32_t>(*pos_x),
                          .pos_y = static_cast<uint32_t>(*pos_y),
                          .hp = static_cast<uint32_t>(*hp)};
}

CborPtr serialize(const GameMapSnapshot& map) {
    auto barriers_cbor = serializeVector(map.barriers);
    if (!barriers_cbor)
        return nullptr;

    return buildArray(cbor_build_uint64(map.blc_x),
                      cbor_build_uint64(map.blc_y),
                      cbor_build_uint64(map.trc_x),
                      cbor_build_uint64(map.trc_y),
                      barriers_cbor.release()  // transfer ownership – array now owns it
    );
}

std::optional<GameMapSnapshot> deserialize(cbor_item_t* item, GameMapSnapshot*) {
    if (!item || !cbor_isa_array(item) || cbor_array_size(item) != kGameMapArraySize)
        return std::nullopt;

    auto blc_x = readCborUint(cbor_array_get(item, 0));
    auto blc_y = readCborUint(cbor_array_get(item, 1));
    auto trc_x = readCborUint(cbor_array_get(item, 2));
    auto trc_y = readCborUint(cbor_array_get(item, 3));
    auto barriers_item = CborPtr(cbor_array_get(item, 4));

    if (!blc_x || !blc_y || !trc_x || !trc_y || !barriers_item)
        return std::nullopt;

    auto barriers = deserializeVector<BarrierSnapshot>(barriers_item.get());
    if (!barriers)
        return std::nullopt;

    return GameMapSnapshot{.blc_x = *blc_x,
                           .blc_y = *blc_y,
                           .trc_x = *trc_x,
                           .trc_y = *trc_y,
                           .barriers = std::move(*barriers)};
}

}  // namespace detail

std::optional<ByteBuffer> serializeGameState(const DungeonSnapshot& snapshot) {
    auto root = detail::CborPtr(cbor_new_definite_map(kSnapshotSize));
    if (!root) {
        LOG_ERROR("[serializeGameState] Failed to create CBOR map");
        return std::nullopt;
    }

    // Helper to add any CBOR value under a given key to the root map
    auto addValueToMap = [&root](const char* key, auto* value) -> bool {
        if (!value) {
            LOG_ERROR(std::format("[serializeGameState] Null CBOR value for key '{}'", key));
            return false;
        }
        auto key_cbor = detail::CborPtr(cbor_build_string(key));
        if (!key_cbor) {
            LOG_ERROR(std::format("[serializeGameState] Failed to build CBOR string for key '{}'", key));
            return false;
        }
        if (!cbor_map_add(root.get(), {.key = key_cbor.get(), .value = value})) {
            LOG_ERROR(std::format("[serializeGameState] Failed to add key '{}' to CBOR map", key));
            return false;
        }
        return true;
    };

    // Helper for vectors (uses serializeVector internally)
    auto addVectorToMap = [&](const char* key, const auto& vec) -> bool {
        auto cbor = detail::serializeVector(vec);
        if (!cbor) {
            LOG_ERROR(std::format("[serializeGameState] Failed to serialize vector for key '{}'", key));
            return false;
        }
        return addValueToMap(key, cbor.get());
    };

    // Map
    auto map_cbor = detail::serialize(snapshot.game_map);
    if (!map_cbor) {
        LOG_ERROR("[serializeGameState] Failed to serialize game_map");
        return std::nullopt;
    }
    if (!addValueToMap(kMap, map_cbor.get()))
        return std::nullopt;

    // Players
    if (!addVectorToMap(kPlayers, snapshot.players))
        return std::nullopt;

    // Mobs
    if (!addVectorToMap(kMobs, snapshot.mobs))
        return std::nullopt;

    return cborToMessage(std::move(root));
}

std::optional<DungeonSnapshot> deserializeGameState(const ByteBuffer& message) {
    cbor_load_result load_result{};
    auto root = detail::bufferToCbor(message, &load_result);

    if (!root || load_result.error.code != CBOR_ERR_NONE || !cbor_isa_map(root.get())) {
        LOG_ERROR("[deserializeGameState] Failed to load CBOR data or not a map");
        return std::nullopt;
    }

    DungeonSnapshot snapshot;

    // Helper: retrieve a CBOR value from the root map by key.
    auto getValue = [&](const char* key) -> detail::CborPtr {
        auto item = detail::CborPtr(detail::cborMapGet(root.get(), key));
        if (!item) {
            LOG_ERROR(std::format("[deserializeGameState] Missing key '{}' in game state", key));
        }
        return item;
    };

    // Helper: extract a single (non‑vector) value by key and deserialize it.
    auto extractValue = [&]<typename T>(const char* key, T& out) -> bool {
        auto item = getValue(key);
        if (!item) {
            return false;
        }
        auto opt = detail::deserialize<T>(item.get());
        if (!opt) {
            LOG_ERROR(std::format("[deserializeGameState] Failed to deserialize value for key '{}'", key));
            return false;
        }
        out = std::move(*opt);
        return true;
    };

    // Helper: extract a vector by key and deserialize it.
    auto extractVector = [&]<typename T>(const char* key, std::vector<T>& out) -> bool {
        auto item = getValue(key);
        if (!item) {
            return false;
        }
        auto opt = detail::deserializeVector<T>(item.get());
        if (!opt) {
            LOG_ERROR(std::format("[deserializeGameState] Failed to deserialize vector for key '{}'", key));
            return false;
        }
        out = std::move(*opt);
        return true;
    };

    // Game map
    if (!extractValue(kMap, snapshot.game_map))
        return std::nullopt;

    // Players
    if (!extractVector(kPlayers, snapshot.players))
        return std::nullopt;

    // Mobs
    if (!extractVector(kMobs, snapshot.mobs))
        return std::nullopt;

    return snapshot;
}

}  // namespace dungeons::common::wire
