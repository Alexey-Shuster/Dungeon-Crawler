#include "serialization_game_state.h"

#include <format>

namespace serialization {

namespace {

inline constexpr size_t kBarrierArraySize = 2;
inline constexpr size_t kEntityArraySize = 6;
inline constexpr size_t kGameMapArraySize = 5;

}  // namespace

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

std::optional<ByteBuffer> serializeGameState(const DungeonSnapshot& snapshot) {
    auto root = CborPtr(cbor_new_definite_map(kBarrierArraySize));
    if (!root) {
        LOG_ERROR("Failed to create CBOR map");
        return std::nullopt;
    }

    // Map
    auto map_cbor = serialize(snapshot.game_map);
    if (!map_cbor)
        return std::nullopt;
    auto key_map = CborPtr(cbor_build_string(kMap));
    if (!key_map || !cbor_map_add(root.get(), {.key = key_map.get(), .value = map_cbor.get()})) {
        return std::nullopt;
    }

    // Entities
    auto entities_cbor = serializeVector(snapshot.entities);
    if (!entities_cbor)
        return std::nullopt;
    auto key_entities = CborPtr(cbor_build_string(kEntities));
    if (!key_entities || !cbor_map_add(root.get(), {.key = key_entities.get(), .value = entities_cbor.get()})) {
        return std::nullopt;
    }

    return cborToMessage(std::move(root));
}
std::optional<DungeonSnapshot> deserializeGameState(const ByteBuffer& message) {
    cbor_load_result load_result{};
    auto root = bufferToCbor(message, &load_result);

    if (!root || load_result.error.code != CBOR_ERR_NONE || !cbor_isa_map(root.get())) {
        LOG_ERROR("Failed to load CBOR data or not a map");
        return std::nullopt;
    }

    DungeonSnapshot snapshot;

    // Map
    auto map_item = CborPtr(cborMapGet(root.get(), kMap));
    if (!map_item) {
        LOG_ERROR(std::format("Missing {} key in game state", std::string_view(kMap)));
        return std::nullopt;
    }
    auto map_opt = deserialize<GameMapSnapshot>(map_item.get());
    if (!map_opt) {
        LOG_ERROR("Failed to deserialize game map");
        return std::nullopt;
    }
    snapshot.game_map = std::move(*map_opt);

    // Entities
    auto entities_item = CborPtr(cborMapGet(root.get(), kEntities));
    if (!entities_item) {
        LOG_ERROR(std::format("Missing {} key in game state", std::string_view(kEntities)));
        return std::nullopt;
    }

    auto entities_opt = deserializeVector<EntitySnapshot>(entities_item.get());
    if (!entities_opt) {
        LOG_ERROR("Failed to deserialize entities");
        return std::nullopt;
    }
    snapshot.entities = std::move(*entities_opt);

    return snapshot;
}

}  // namespace serialization
