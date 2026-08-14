#pragma once

#include <common/game_state_dto.h>
#include <string>
#include <vector>

namespace dungeons::client::ui {

// ---- Entity type codes (see server) ----
constexpr uint8_t kEntityTypePlayer = 0;
constexpr uint8_t kEntityTypeMonster = 1;

std::vector<std::string> renderGameState(const network::DungeonSnapshot& snapshot);

}  // namespace dungeons::client::ui
