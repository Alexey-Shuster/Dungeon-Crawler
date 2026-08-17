#pragma once

#include "serder_vector.h"  // IWYU pragma: keep // serialize & deserialize

namespace dungeons::common::wire {

inline const char* kMap = "map";
inline const char* kPlayers = "players";
inline const char* kMobs = "mobs";

std::optional<network::ByteBuffer> serializeGameState(const network::DungeonSnapshot& snapshot);

std::optional<network::DungeonSnapshot> deserializeGameState(const network::ByteBuffer& message);

}  // namespace dungeons::common::wire
