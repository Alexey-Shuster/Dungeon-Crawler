#include "create_game_state.h"

namespace dungeons::server::domain {

common::network::DungeonSnapshot createGameStateDTO(const DungeonState& state) {
    common::network::DungeonSnapshot snapshot{};

    // 1. Map snapshot
    const auto& map_size = state.game_map.size();
    snapshot.game_map.blc_x = static_cast<uint64_t>(map_size.getBottomLeftCorner().x);
    snapshot.game_map.blc_y = static_cast<uint64_t>(map_size.getBottomLeftCorner().y);
    snapshot.game_map.trc_x = static_cast<uint64_t>(map_size.getTopRightCorner().x);
    snapshot.game_map.trc_y = static_cast<uint64_t>(map_size.getTopRightCorner().y);

    // Barriers
    snapshot.game_map.barriers.reserve(state.game_map.getBarriers().size());
    for (const auto& pos : state.game_map.getBarriers()) {
        snapshot.game_map.barriers.push_back(
            {.pos_x = static_cast<uint64_t>(pos.x), .pos_y = static_cast<uint64_t>(pos.y)});
    }

    // 2. Entity snapshots
    snapshot.players.reserve(state.players.size());
    snapshot.mobs.reserve(state.monsters.size());

    // Helper to build an EntitySnapshot
    auto make_entity_snapshot = [](auto id, const auto& entity, uint8_t type) {
        return common::network::EntitySnapshot{
            .type = type,
            .state = static_cast<uint8_t>(entity.isAlive() ? EntityState::Alive : EntityState::Dead),
            .id = id.value,
            .pos_x = static_cast<uint64_t>(entity.GetPosition().x),
            .pos_y = static_cast<uint64_t>(entity.GetPosition().y),
            .hp = entity.getHealth().value()
        };
    };

    for (const auto& [id, player] : state.players) {
        snapshot.players.push_back(
            make_entity_snapshot(id, player, static_cast<uint8_t>(EntityType::Player)));
    }

    for (const auto& [id, monster] : state.monsters) {
        snapshot.mobs.push_back(
            make_entity_snapshot(id, monster, static_cast<uint8_t>(EntityType::Monster)));
    }

    return snapshot;
}

}  // namespace dungeons::server::domain
