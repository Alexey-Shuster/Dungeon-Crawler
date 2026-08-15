#pragma once

#include <common/network/game_state_dto.h>

#include "dungeon.h"

namespace dungeons::server::domain {

common::network::DungeonSnapshot createGameStateDTO(const DungeonState& state);

}  // namespace dungeons::server::domain
