#pragma once

#include <cstdint> // IWYU pragma: keep // uint8_t

namespace entity {

enum class EntityState : std::uint8_t { Alive = 0, Dead };
enum class EntityType : std::uint8_t { Player = 0, Monster };

}
