#include "command_router.h"

#include <common/types/direction.h>
#include <common/utility/hash.h>
#include <common/wire/serialization.h>
#include <common/utility/string_utils.h>
#include <format>
#include <sstream>
#include <string_view>

#include "client_commands.h"

namespace dungeons::client::ui {

namespace {

template <typename MsgType, typename... Args>
std::optional<common::network::RawMessage> trySerialize(MsgType type, const ConsoleOutput& out, Args&&... args) {
    auto optBuffer = common::wire::serializeMessage(type, std::forward<Args>(args)...);
    if (!optBuffer) {
        out(std::format("Failed to serialize {} message", commandFromMessageType(type)));
        return std::nullopt;
    }
    return common::network::RawMessage{std::move(*optBuffer)};
}

constexpr std::string_view kUsage = "Usage:";
constexpr std::string_view kUsername = "<username>";
constexpr std::string_view kLobbyID = "<lobbyID>";
constexpr std::string_view kDirection = "<direction>";

constexpr uint8_t kMinLenPlayerName = 4;
constexpr uint8_t kMaxLenPlayerName = 10;

}  // namespace

std::optional<common::network::RawMessage> handleJoin(std::string_view name, const ConsoleOutput& out) {
    if (name.size() < kMinLenPlayerName || name.size() > kMaxLenPlayerName) {
        out(std::format("Invalid name: must be >{} & <={} english alphanumeric characters",
                        kMinLenPlayerName,
                        kMaxLenPlayerName));
        return std::nullopt;
    }

    auto playerId = common::utility::EncodeString(name);
    if (!playerId) {
        out("Failed to encode player ID. tip: only english alphanumeric characters.");
        return std::nullopt;
    }

    return trySerialize(common::types::NetworkMessageType::kJoin, out, *playerId);
}

std::optional<common::network::RawMessage> handleJoinLobby(std::string_view lobby, const ConsoleOutput& out) {
    auto opt_lobbyId = common::utility::convertStringToUint64(lobby);
    if (!opt_lobbyId) {
        out("Invalid lobby ID. tip: positive number under uint64_t.");
        return std::nullopt;
    }

    return trySerialize(common::types::AppMessageType::kJoinParty, out, *opt_lobbyId);
}

std::optional<common::network::RawMessage> routeCommand(const std::string& line, const ConsoleOutput& output) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    using namespace common::types;
    using namespace common::utility;

    if (cmd == commands::kJoin) {
        std::string name;
        if (!(iss >> name)) {
            output(joinWithSpace(kUsage, commands::kJoin, kUsername));
            return std::nullopt;
        }
        return handleJoin(name, output);
    } else if (cmd == commands::kCreateLobby) {
        return trySerialize(AppMessageType::kCreateParty, output);
    } else if (cmd == commands::kListLobby) {
        return trySerialize(AppMessageType::kListParties, output);
    } else if (cmd == commands::kJoinLobby) {
        std::string lobby;
        if (!(iss >> lobby)) {
            output(joinWithSpace(kUsage, commands::kJoinLobby, kLobbyID));
            return std::nullopt;
        }
        return handleJoinLobby(lobby, output);
    } else if (cmd == commands::kStartGame) {
        return trySerialize(AppMessageType::kStartGame, output);
    } else if (cmd == commands::kMove) {
        std::string dir_str;
        if (!(iss >> dir_str)) {
            output(joinWithSpace(kUsage, commands::kMove, kDirection));
            return std::nullopt;
        }
        if (auto opt_dir = directionFromString(dir_str)) {
            return trySerialize(DomainMessageType::kMove, output, directionToByte(*opt_dir).value());
        }
        output(commands::kUnknown);
        return std::nullopt;
    } else if (cmd == commands::kAttack) {
        return trySerialize(DomainMessageType::kAttack, output);
    }

    output(commands::kUnknown);
    return std::nullopt;
}

}  // namespace dungeons::client::ui
