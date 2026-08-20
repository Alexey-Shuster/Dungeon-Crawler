#pragma once

#include <common/network/byte_buffer.h>
#include <common/network/game_state_dto.h>
#include <optional>

namespace dungeons::common::wire {

inline const char* kMap = "map";
inline const char* kPlayers = "players";
inline const char* kMobs = "mobs";

std::optional<network::ByteBuffer> serializeGameState(const network::DungeonSnapshot& snapshot);

std::optional<network::DungeonSnapshot> deserializeGameState(const network::ByteBuffer& message);

}  // namespace dungeons::common::wire
