#pragma once

#include "common/game_state_dto.h"
#include "dungeon.h"

namespace dungeons::server::domain {

serialization::DungeonSnapshot createGameStateDTO(const DungeonState& state);

}  // namespace dungeons::server::domain
