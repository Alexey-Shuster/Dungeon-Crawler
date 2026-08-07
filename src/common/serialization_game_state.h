#pragma once

#include <cbor.h>
#include <optional>

#include "../domain/game_state_dto.h"
#include "serialization_utils.h"
#include "serialization_vector.h"  // IWYU pragma: keep // serialize & deserialize

namespace serialization {

inline const char* kMap = "map";
inline const char* kEntities = "entities";

CborPtr serialize(const BarrierSnapshot& barrier);

std::optional<BarrierSnapshot> deserialize(cbor_item_t* item, BarrierSnapshot*);

CborPtr serialize(const EntitySnapshot& entity);

std::optional<EntitySnapshot> deserialize(cbor_item_t* item, EntitySnapshot*);

CborPtr serialize(const GameMapSnapshot& map);

std::optional<GameMapSnapshot> deserialize(cbor_item_t* item, GameMapSnapshot*);

// Top‑level DungeonSnapshot
std::optional<network::Message> serializeGameState(const DungeonSnapshot& snapshot);

std::optional<DungeonSnapshot> deserializeGameState(const network::Message& message);

}  // namespace serialization
