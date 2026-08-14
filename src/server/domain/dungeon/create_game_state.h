#pragma once

#include "common/game_state_dto.h"
#include "dungeon.h"

namespace dungeons::server::domain {

network::DungeonSnapshot createGameStateDTO(const DungeonState& state);

}  // namespace dungeons::server::domain
