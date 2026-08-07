#include "command_router.h"

#include <format>
#include <sstream>
#include <string_view>
#include <vector>

#include "client_commands.h"
#include "direction.h"
#include "hash.h"
#include "serialization.h"
#include "string_utils.h"

namespace {

template <typename MsgType, typename... Args>
std::optional<network::MessageData> trySerialize(MsgType type, const network::ConsoleOutput& out, Args&&... args) {
    auto optMsg = serialization::serializeMessage(type, std::forward<Args>(args)...);
    if (!optMsg) {
        out(std::format("Failed to serialize {} message", network::client_commands::commandFromMessageType(type)));
        return std::nullopt;
    }
    return optMsg->message_data;
}

constexpr std::string_view kUsage = "Usage:";
constexpr std::string_view kUsername = "<username>";
constexpr std::string_view kLobbyID = "<lobbyID>";
constexpr std::string_view kDirection = "<direction>";
constexpr std::string_view kUnknown = "Unknown command";

}  // namespace

namespace network::command_router {

static constexpr uint8_t kMinLenPlayerName = 4;
static constexpr uint8_t kMaxLenPlayerName = 10;

std::optional<MessageData> handleJoin(std::string_view name, const ConsoleOutput& out) {
    if (name.size() < kMinLenPlayerName || name.size() > kMaxLenPlayerName) {
        out(std::format("Invalid name: must be >{} & <={} english alphanumeric characters",
                        kMinLenPlayerName,
                        kMaxLenPlayerName));
        return std::nullopt;
    }

    auto playerId = hash::EncodeString(name);
    if (!playerId) {
        out("Failed to encode player ID. tip: only english alphanumeric characters.");
        return std::nullopt;
    }

    return trySerialize(message::NetworkMessageType::kJoin, out, *playerId);
}

std::optional<MessageData> handleJoinLobby(std::string_view lobby, const ConsoleOutput& out) {
    auto opt_lobbyId = utility::convertStringToUint64(lobby);
    if (!opt_lobbyId) {
        out("Invalid lobby ID. tip: positive number under uint64_t.");
        return std::nullopt;
    }

    return trySerialize(message::AppMessageType::kJoinParty, out, *opt_lobbyId);
}

std::optional<MessageData> route(const std::string& line, const ConsoleOutput& output) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == client_commands::kJoin) {
        std::string name;
        if (!(iss >> name)) {
            output(utility::joinWithSpace(kUsage, client_commands::kJoin, kUsername));
            return std::nullopt;
        }
        return handleJoin(name, output);
    } else if (cmd == client_commands::kCreateLobby) {
        return trySerialize(message::AppMessageType::kCreateParty, output);
    } else if (cmd == client_commands::kListLobby) {
        return trySerialize(message::AppMessageType::kListParties, output);
    } else if (cmd == client_commands::kJoinLobby) {
        std::string lobby;
        if (!(iss >> lobby)) {
            output(utility::joinWithSpace(kUsage, client_commands::kJoinLobby, kLobbyID));
            return std::nullopt;
        }
        return handleJoinLobby(lobby, output);
    } else if (cmd == client_commands::kStartGame) {
        return trySerialize(message::AppMessageType::kStartGame, output);
    } else if (cmd == client_commands::kMove) {
        std::string dir_str;
        if (!(iss >> dir_str)) {
            output(utility::joinWithSpace(kUsage, client_commands::kMove, kDirection));
            return std::nullopt;
        }
        if (auto opt_dir = directionFromString(dir_str)) {
            return trySerialize(message::DomainMessageType::kMove, output, static_cast<uint8_t>(*opt_dir));
        }
        output(kUnknown);
        return std::nullopt;
    } else if (cmd == client_commands::kAttack) {
        return trySerialize(message::DomainMessageType::kAttack, output);
    }

    output(kUnknown);
    return std::nullopt;
}

}  // namespace network::command_router
