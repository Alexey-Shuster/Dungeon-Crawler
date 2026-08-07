#pragma once

#include "dungeon/dungeon.h"
#include "game_state_dto.h"

namespace serialization {

DungeonSnapshot createGameStateDTO(const dungeon::DungeonState& state);

}  // namespace serialization
