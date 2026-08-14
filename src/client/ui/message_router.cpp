#include "message_router.h"

#include <common/hash.h>
#include <common/message.h>
#include <common/serialization.h>
#include <common/serialization_game_state.h>
#include <common/string_utils.h>
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
    if (args.empty()) {
        return utility::joinWithSpace(prefix, kMalformed);
    }

    auto success = [&](const std::string& str) -> std::string {
        return utility::joinWithSpace(prefix, id_type, str);
    };

    auto failure = [&](std::string_view error) -> std::string {
        return utility::joinWithSpace(prefix, error);
    };

    if (id_type == kPlayerId) {
        if (auto name = hash::DecodeString(args[0])) {
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

void routeMessage(::network::Message data, const ConsoleOutput& output) {
    auto game_state = network::deserializeGameState(data.buffer);
    if (game_state.has_value()) {
        auto game_state_view = renderGameState(*game_state);
        for (const auto& line : game_state_view) {
            output(line);
        }
        return;
    }

    auto parsed = network::deserializeMessageRaw(std::move(data.buffer));
    if (!parsed) {
        output("Failed to parse incoming message");
        return;
    }

    auto opt_Type = parsed->type;

    if (message::isNetworkMessage(opt_Type)) {
        switch (*message::asNetworkMessage(opt_Type)) {
            case message::NetworkMessageType::kWelcome:
                output("WELCOME");
                break;
            case message::NetworkMessageType::kAuthFailed:
                output(formatOutput(kAuthFailed, parsed->args, kPlayerId));
                break;
            default:
                output("Unknown network message");
                break;
        }
    } else if (message::isAppMessage(opt_Type)) {
        switch (*message::asAppMessage(opt_Type)) {
            case message::AppMessageType::kPartyCreated:
                output(formatOutput(kLobbyCreated, parsed->args, kLobbyId));
                break;
            case message::AppMessageType::kPartyNotCreated:
                output("LOBBY NOT CREATED");
                break;
            case message::AppMessageType::kListPartiesCreated:
                output(formatOutput(kLobbyListCreated, parsed->args, kLobbyList));
                break;
            case message::AppMessageType::kListPartiesNotCreated:
                output("LOBBY LIST NOT CREATED");
                break;
            case message::AppMessageType::kPlayerJoinedParty:
                output("JOIN LOBBY SUCCESS");
                break;
            case message::AppMessageType::kPlayerNotJoinedParty:
                output("JOIN LOBBY FAILED");
                break;
            case message::AppMessageType::kPlayerNotJoinedFullParty:
                output("JOIN LOBBY FAILED - LOBBY FULL");
                break;
            case message::AppMessageType::kPlayerNotJoinedNotExistParty:
                output("JOIN LOBBY FAILED - LOBBY NOT EXIST");
                break;
            case message::AppMessageType::kGameStarted:
                output("START GAME SUCCESS");
                break;
            case message::AppMessageType::kGameNotStarted:
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
