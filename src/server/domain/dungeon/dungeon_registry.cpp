#include "dungeon_registry.h"

#include <common/utility/config.h>
#include <common/utility/logger.h>
#include <format>
#include <ranges>
#include <unordered_set>

#include "dungeon.h"

namespace dungeons::server::domain {

bool DungeonRegistry::addDungeon(GameMap game_map, std::vector<PlayerId> player_ids) {
    if (player_ids.empty()) {
        LOG_ERROR("Cannot create Dungeon with no players");
        return false;
    }

    // Check for duplicates in the input list
    std::unordered_set<PlayerId, PlayerHash> unique_check(player_ids.begin(),
                                                                                      player_ids.end());
    if (unique_check.size() != player_ids.size()) {
        LOG_ERROR("Duplicate player IDs in the input list");
        return false;
    }

    // Check if ANY player is already in another dungeon
    for (PlayerId pid : player_ids) {
        if (player_to_dungeon_.contains(pid)) {
            LOG_ERROR(std::format("Player {} is already in another dungeon with GameId {}",
                                  pid.value,
                                  player_to_dungeon_.at(pid).value));
            return false;
        }
    }

    auto dungeon = std::make_shared<Dungeon>(game_map, player_ids);
    GameId id = generateNextId();

    // Insert into primary maps
    auto [it, inserted] = dungeons_.emplace(id, dungeon);
    if (!inserted) {
        LOG_ERROR(std::format("Failed to register dungeon #{}", id.value));
        return false;
    }

    // Update reverse lookups (already validated above)
    for (PlayerId pid : player_ids) {
        player_to_dungeon_.emplace(pid, id);
    }

    // Store the player list for this dungeon
    size_t num_players = player_ids.size();
    dungeon_to_players_.emplace(id, std::move(player_ids));

    LOG_INFO(std::format("Dungeon #{} created with {} players.", id.value, num_players));
    return true;
}

bool DungeonRegistry::removeDungeon(GameId id) {
    auto it = dungeons_.find(id);
    if (it == dungeons_.end()) {
        LOG_ERROR(std::format("Dungeon #{} not found", id.value));
        return false;
    }

    // Get the exact list of players for this dungeon
    auto players_it = dungeon_to_players_.find(id);
    if (players_it != dungeon_to_players_.end()) {
        // Erase each player from the global player->dungeon map (O(K))
        for (PlayerId pid : players_it->second) {
            player_to_dungeon_.erase(pid);
        }
        // Erase the player list entry
        dungeon_to_players_.erase(players_it);
    }

    // Erase the dungeon itself
    dungeons_.erase(it);
    LOG_INFO(std::format("Dungeon #{} removed", id.value));

    return true;
}

std::shared_ptr<Dungeon> DungeonRegistry::findDungeon(GameId id) const {
    auto it = dungeons_.find(id);
    return (it != dungeons_.end()) ? it->second : nullptr;
}

std::optional<GameId> DungeonRegistry::findPlayerDungeon(PlayerId player_id) const {
    auto it = player_to_dungeon_.find(player_id);
    if (it == player_to_dungeon_.end()) {
        LOG_ERROR(std::format("No Dungeon found for player #{}", player_id.value));
        return std::nullopt;
    }
    return it->second;
}

const std::unordered_map<GameId, std::shared_ptr<Dungeon>, GameHash>&
DungeonRegistry::getAllDungeons() {
    return dungeons_;
}

GameId DungeonRegistry::generateNextId() {
    return next_game_id_++;
}

}  // namespace dungeons::server::domain
