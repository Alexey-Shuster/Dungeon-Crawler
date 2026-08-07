#pragma once

#include <string>
#include <vector>

#include "game_state_dto.h"

namespace serialization {

// ---- Entity type codes (see server) ----
constexpr uint8_t kEntityTypePlayer = 0;
constexpr uint8_t kEntityTypeMonster = 1;

std::vector<std::string> renderGameState(const DungeonSnapshot& snapshot);

}  // namespace serialization
