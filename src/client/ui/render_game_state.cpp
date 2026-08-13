#include "render_game_state.h"

#include <cstdint>  // IWYU pragma: keep // uint64_t
#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "../../common/logger.h"
#include "../../common/number_utils.h"
#include "../../common/string_utils.h"
#include "../../domain/game_state_dto.h"

namespace serialization {

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

std::vector<std::string> renderGameState(const DungeonSnapshot& snapshot) {
    const auto& map = snapshot.game_map;

    uint64_t minX = map.blc_x;
    uint64_t maxX = map.trc_x;
    uint64_t minY = map.blc_y;
    uint64_t maxY = map.trc_y;

    if (maxX < minX || maxY < minY) {
        std::string msg = utility::joinWithSpace(kRenderGameState, kInvalidCoordinates);
        LOG_INFO(msg);
        return {msg};
    }

    uint64_t diffX = maxX - minX;
    uint64_t diffY = maxY - minY;

    if (diffX == std::numeric_limits<uint64_t>::max() || diffY == std::numeric_limits<uint64_t>::max()) {
        std::string msg = utility::joinWithSpace(kRenderGameState, kCoordinateOverflow);
        LOG_INFO(msg);
        return {msg};
    }

    uint64_t w64 = diffX + 1;
    uint64_t h64 = diffY + 1;

    if (std::cmp_greater(w64, kMaxMapCells / h64)) {
        std::string msg = utility::joinWithSpace(kRenderGameState, kTooLargeMsg, kMaxMapCells);
        LOG_INFO(msg);
        return {msg};
    }

    size_t width = static_cast<size_t>(w64);
    size_t height = static_cast<size_t>(h64);

    // Initialise grid with floor
    std::vector<std::string> grid(height, std::string(width, kFloorSymbol));

    // Place barriers
    for (const auto& barrier : map.barriers) {
        uint64_t x = barrier.pos_x;
        uint64_t y = barrier.pos_y;
        if (utility::isBetween(x, minX, maxX) && utility::isBetween(y, minY, maxY)) {
            grid[static_cast<size_t>(y - minY)][static_cast<size_t>(x - minX)] = kBarrierSymbol;
        }
    }

    // Place entities (overwrites barriers)
    for (const auto& entity : snapshot.entities) {
        uint64_t x = entity.pos_x;
        uint64_t y = entity.pos_y;
        if (utility::isBetween(x, minX, maxX) && utility::isBetween(y, minY, maxY)) {
            char symbol = kUnknownSymbol;
            switch (entity.type) {
                case kEntityTypePlayer:
                    symbol = kPlayerSymbol;
                    break;
                case kEntityTypeMonster:
                    symbol = kMonsterSymbol;
                    break;
                default:
                    break;
            }
            grid[static_cast<size_t>(y - minY)][static_cast<size_t>(x - minX)] = symbol;
        }
    }

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
    lines.emplace_back(std::format("{} {}", kEntitiesMsg, snapshot.entities.size()));

    // Display HP info for each entity
    for (const auto& entity : snapshot.entities) {
        std::string_view typeStr;
        switch (entity.type) {
            case kEntityTypePlayer:
                typeStr = kPlayerTypeString;
                break;
            case kEntityTypeMonster:
                typeStr = kMonsterTypeString;
                break;
            default:
                typeStr = kUnknownTypeString;
                break;
        }
        lines.emplace_back(std::format("  Entity {} ({}) HP: {}", entity.id, typeStr, entity.hp));
    }

    return lines;
}

}  // namespace serialization
