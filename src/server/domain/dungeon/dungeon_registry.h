#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "map/game_map.h"
#include "core/types.h"
#include "dungeon_fwd.h"

namespace dungeons::server::domain {

// This class is not thread-safe. All methods must be called from a
// single thread, or external synchronization must be provided.

class DungeonRegistry {
public:
    DungeonRegistry() = default;
    ~DungeonRegistry() = default;

    DungeonRegistry(const DungeonRegistry&) = delete;
    DungeonRegistry& operator=(const DungeonRegistry&) = delete;

    [[nodiscard]] bool addDungeon(GameMap game_map, std::vector<PlayerId> player_ids);

    [[nodiscard]] bool removeDungeon(GameId id);

    [[nodiscard]] std::shared_ptr<Dungeon> findDungeon(GameId id) const;

    [[nodiscard]] std::optional<GameId> findPlayerDungeon(PlayerId player_id) const;

    [[nodiscard]] const std::unordered_map<GameId, std::shared_ptr<Dungeon>, GameHash>& getAllDungeons();

private:
    std::unordered_map<GameId, std::shared_ptr<Dungeon>, GameHash> dungeons_;
    std::unordered_map<PlayerId, GameId, PlayerHash> player_to_dungeon_;
    std::unordered_map<GameId, std::vector<PlayerId>, GameHash> dungeon_to_players_;

    GameId next_game_id_{GameId{1}};

    GameId generateNextId();
};

}  // namespace dungeons::server::domain
