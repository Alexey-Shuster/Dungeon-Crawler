#include "render_game_state.h"

#include <common/utility/logger.h>
#include <common/utility/number_utils.h>
#include <common/utility/string_utils.h>
#include <cstdint>  // IWYU pragma: keep // uint64_t
#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace dungeons::client::ui {

namespace {

// ---- ASCII symbols ----
constexpr char kPlayerSymbol = '@';
constexpr char kMonsterSymbol = 'M';
constexpr char kOtherPlayerSymbol = 'C';
constexpr char kBarrierSymbol = 'b';
constexpr char kFloorSymbol = '.';
constexpr char kUnknownSymbol = '?';
constexpr char kVerticalBorder = '#';
constexpr char kHorizontalBorder = '#';
constexpr char kCorner = '#';

// ---- Entity type strings ----
constexpr std::string_view kPlayerTypeString = "Player";
constexpr std::string_view kMonsterTypeString = "Monster";
constexpr std::string_view kUnknownTypeString = "Unknown";

// ---- UI strings ----
constexpr std::string_view kRenderGameState = "[renderGameState]";
constexpr std::string_view kInvalidCoordinates = "Invalid coordinates";
constexpr std::string_view kCoordinateOverflow = "Coordinate overflow";
constexpr std::string_view kHeaderMsg = "=== Game State ===";
constexpr std::string_view kEntitiesMsg = "Entities:";
constexpr std::string_view kTooLargeMsg = "Game map too large to display: width*height > ";

// ---- Safety limit ----
constexpr size_t kMaxMapCells = 200 * 200;

}  // namespace

std::vector<std::string> renderGameState(const common::network::DungeonSnapshot& snapshot) {
    const auto& map = snapshot.game_map;

    uint64_t minX = map.blc_x;
    uint64_t maxX = map.trc_x;
    uint64_t minY = map.blc_y;
    uint64_t maxY = map.trc_y;

    using namespace common::utility;

    if (maxX < minX || maxY < minY) {
        std::string msg = joinWithSpace(kRenderGameState, kInvalidCoordinates);
        LOG_INFO(msg);
        return {msg};
    }

    uint64_t diffX = maxX - minX;
    uint64_t diffY = maxY - minY;

    if (diffX == std::numeric_limits<uint64_t>::max() || diffY == std::numeric_limits<uint64_t>::max()) {
        std::string msg = joinWithSpace(kRenderGameState, kCoordinateOverflow);
        LOG_INFO(msg);
        return {msg};
    }

    uint64_t w64 = diffX + 1;
    uint64_t h64 = diffY + 1;

    if (std::cmp_greater(w64, kMaxMapCells / h64)) {
        std::string msg = joinWithSpace(kRenderGameState, kTooLargeMsg, kMaxMapCells);
        LOG_INFO(msg);
        return {msg};
    }

    size_t width = static_cast<size_t>(w64);
    size_t height = static_cast<size_t>(h64);

    // Initialize grid with floor
    std::vector<std::string> grid(height, std::string(width, kFloorSymbol));

    // Helper to place any item type that has pos_x / pos_y onto the grid
    auto placeOnGrid = [&](const auto& items, char symbol) {
        for (const auto& item : items) {
            uint64_t x = item.pos_x;
            uint64_t y = item.pos_y;
            if (isBetween(x, minX, maxX) && isBetween(y, minY, maxY)) {
                grid[static_cast<size_t>(y - minY)][static_cast<size_t>(x - minX)] = symbol;
            }
        }
    };

    // Place barriers, players, and mobs
    placeOnGrid(map.barriers, kBarrierSymbol);
    placeOnGrid(snapshot.players, kPlayerSymbol);
    placeOnGrid(snapshot.mobs, kMonsterSymbol);

    std::vector<std::string> lines;
    lines.reserve(height + 4);

    lines.emplace_back(kHeaderMsg);

    std::string border(width + 2, kHorizontalBorder);
    border.front() = kCorner;
    border.back() = kCorner;
    lines.push_back(border);

    // Print rows from top (maxY) to bottom (minY)
    for (int64_t y = maxY; y >= static_cast<int64_t>(minY); --y) {
        size_t idx = static_cast<size_t>(y - minY);
        lines.emplace_back(kVerticalBorder + grid[idx] + kVerticalBorder);
    }

    lines.push_back(border);
    size_t totalEntities = snapshot.players.size() + snapshot.mobs.size();
    lines.emplace_back(std::format("{} {}", kEntitiesMsg, totalEntities));

    // Display HP info for each entity
    for (const auto& player : snapshot.players) {
        lines.emplace_back(std::format("  Entity {} ({}) HP: {}", player.id, kPlayerTypeString, player.hp));
    }
    for (const auto& mob : snapshot.mobs) {
        lines.emplace_back(std::format("  Entity {} ({}) HP: {}", mob.id, kMonsterTypeString, mob.hp));
    }

    return lines;
}

}  // namespace dungeons::client::ui
