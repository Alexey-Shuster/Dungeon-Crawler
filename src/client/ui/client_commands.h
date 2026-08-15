#pragma once

#include <common/types/message_types.h>
#include <string_view>

namespace dungeons::client::ui {

namespace commands {
inline constexpr std::string_view kJoin = "JOIN";
inline constexpr std::string_view kCreateLobby = "CREATE_LOBBY";
inline constexpr std::string_view kListLobby = "LIST_LOBBY";
inline constexpr std::string_view kJoinLobby = "JOIN_LOBBY";
inline constexpr std::string_view kStartGame = "START_GAME";
inline constexpr std::string_view kMove = "MOVE";
inline constexpr std::string_view kAttack = "ATTACK";
inline constexpr std::string_view kUnknown = "UNKNOWN_COMMAND";
}  // namespace commands

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

[[nodiscard]] inline std::string_view commandFromMessageType(const common::types::MessageTypeVariant& type) noexcept {
    using namespace common::types;
    using namespace commands;
    return std::visit(overloaded{[](NetworkMessageType net_type) noexcept -> std::string_view {
                                     switch (net_type) {
                                         case NetworkMessageType::kJoin:
                                             return kJoin;
                                         default:
                                             return kUnknown;
                                     }
                                 },
                                 [](AppMessageType app_type) noexcept -> std::string_view {
                                     switch (app_type) {
                                         case AppMessageType::kCreateParty:
                                             return kCreateLobby;
                                         case AppMessageType::kListParties:
                                             return kListLobby;
                                         case AppMessageType::kJoinParty:
                                             return kJoinLobby;
                                         case AppMessageType::kStartGame:
                                             return kStartGame;
                                         default:
                                             return kUnknown;
                                     }
                                 },
                                 [](DomainMessageType domain_type) noexcept -> std::string_view {
                                     switch (domain_type) {
                                         case DomainMessageType::kMove:
                                             return kMove;
                                         case DomainMessageType::kAttack:
                                             return kAttack;
                                         default:
                                             return kUnknown;
                                     }
                                 },
                                 [](const auto&) noexcept -> std::string_view {
                                     return kUnknown;
                                 }},
                      type);
}

}  // namespace dungeons::client::ui
