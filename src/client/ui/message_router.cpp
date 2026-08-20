#include "message_router.h"

#include <common/network/raw_message.h>
#include <common/types/message_utils.h>
#include <common/utility/hash.h>
#include <common/utility/string_utils.h>
#include <common/wire/serder.h>
#include <common/wire/serder_game_state.h>
#include <string_view>
#include <vector>

#include "render_game_state.h"

namespace dungeons::client::ui {

namespace {

constexpr std::string_view kMalformed = "malformed message argumnets";
constexpr std::string_view kUnknown = "unknown argumnets type";
constexpr std::string_view kPlayerIdDecodeError = "PlayerID decode error";
constexpr std::string_view kLobbyIdError = "LobbyId error";
constexpr std::string_view kLobbyListError = "LobbyList error";

constexpr std::string_view kAuthFailed = "AUTHENTICATION FAILED";
constexpr std::string_view kLobbyCreated = "LOBBY CREATED";
constexpr std::string_view kLobbyListCreated = "LOBBY LIST CREATED";

constexpr std::string_view kPlayerId = "PlayerId";
constexpr std::string_view kLobbyId = "LobbyId";
constexpr std::string_view kLobbyList = "LobbyList";

}  // namespace

// Centralized error formatter
static std::string formatOutput(std::string_view prefix, const std::vector<uint64_t>& args, std::string_view id_type) {
    using namespace common::utility;

    if (args.empty()) {
        return joinWithSpace(prefix, kMalformed);
    }

    auto success = [&](const std::string& str) -> std::string {
        return joinWithSpace(prefix, id_type, str);
    };

    auto failure = [&](std::string_view error) -> std::string {
        return joinWithSpace(prefix, error);
    };

    if (id_type == kPlayerId) {
        if (auto name = DecodeString(args[0])) {
            return success(*name);
        } else {
            return failure(kPlayerIdDecodeError);
        }
    } else if (id_type == kLobbyId) {
        return success(std::to_string(args[0]));
    } else if (id_type == kLobbyList) {
        std::string list;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0)
                list += ',';
            list += std::to_string(args[i]);
        }
        return success(list);
    } else {
        // Unknown id_type – treat as error (or fall back to malformed)
        return failure(kUnknown);
    }
}

void routeMessage(common::network::RawMessage data, const ConsoleOutput& output) {
    auto game_state = common::wire::deserializeGameState(data.buffer);
    if (game_state.has_value()) {
        auto game_state_view = renderGameState(*game_state);
        for (const auto& line : game_state_view) {
            output(line);
        }
        return;
    }

    auto parsed = common::wire::deserializeBufferToMessage(std::move(data.buffer));
    if (!parsed) {
        output("Failed to parse incoming message");
        return;
    }

    auto opt_Type = parsed->type;
    using namespace common::types;

    if (isNetworkMessage(opt_Type)) {
        switch (*asNetworkMessage(opt_Type)) {
            case NetworkMessageType::kWelcome:
                output("WELCOME");
                break;
            case NetworkMessageType::kAuthFailed:
                output(formatOutput(kAuthFailed, parsed->args, kPlayerId));
                break;
            default:
                output("Unknown network message");
                break;
        }
    } else if (isAppMessage(opt_Type)) {
        switch (*asAppMessage(opt_Type)) {
            case AppMessageType::kPartyCreated:
                output(formatOutput(kLobbyCreated, parsed->args, kLobbyId));
                break;
            case AppMessageType::kPartyNotCreated:
                output("LOBBY NOT CREATED");
                break;
            case AppMessageType::kListPartiesCreated:
                output(formatOutput(kLobbyListCreated, parsed->args, kLobbyList));
                break;
            case AppMessageType::kListPartiesNotCreated:
                output("LOBBY LIST NOT CREATED");
                break;
            case AppMessageType::kPlayerJoinedParty:
                output("JOIN LOBBY SUCCESS");
                break;
            case AppMessageType::kPlayerNotJoinedParty:
                output("JOIN LOBBY FAILED");
                break;
            case AppMessageType::kPlayerNotJoinedFullParty:
                output("JOIN LOBBY FAILED - LOBBY FULL");
                break;
            case AppMessageType::kPlayerNotJoinedNotExistParty:
                output("JOIN LOBBY FAILED - LOBBY NOT EXIST");
                break;
            case AppMessageType::kGameStarted:
                output("START GAME SUCCESS");
                break;
            case AppMessageType::kGameNotStarted:
                output("START GAME FAILED");
                break;
            default:
                output("Unknown app message");
                break;
        }
    } else {
        output("Unsupported message level");
    }
}

}  // namespace dungeons::client::ui
