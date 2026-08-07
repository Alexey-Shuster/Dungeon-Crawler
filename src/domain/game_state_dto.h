#pragma once

#include <compare>  // IWYU pragma: keep // operator<=>
#include <cstdint>
#include <vector>

namespace serialization {

struct EntitySnapshot {
    uint8_t type;
    uint8_t state;
    uint64_t id;
    uint64_t pos_x;
    uint64_t pos_y;
    uint32_t hp;

    auto operator<=>(const EntitySnapshot&) const = default;
};

struct BarrierSnapshot {
    uint64_t pos_x;
    uint64_t pos_y;

    auto operator<=>(const BarrierSnapshot&) const = default;
};

struct GameMapSnapshot {
    uint64_t blc_x;
    uint64_t blc_y;
    uint64_t trc_x;
    uint64_t trc_y;
    std::vector<BarrierSnapshot> barriers;

    auto operator<=>(const GameMapSnapshot&) const = default;
};

struct DungeonSnapshot {
    GameMapSnapshot game_map;
    std::vector<EntitySnapshot> entities;

    auto operator<=>(const DungeonSnapshot&) const = default;
};

}  // namespace serialization
