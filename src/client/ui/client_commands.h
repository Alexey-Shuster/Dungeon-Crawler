#pragma once

#include <common/message_types.h>
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

[[nodiscard]] inline std::string_view commandFromMessageType(const message::MessageTypeVariant& type) noexcept {
    using namespace commands;
    return std::visit(overloaded{[](message::NetworkMessageType net_type) noexcept -> std::string_view {
                                     switch (net_type) {
                                         case message::NetworkMessageType::kJoin:
                                             return kJoin;
                                         default:
                                             return kUnknown;
                                     }
                                 },
                                 [](message::AppMessageType app_type) noexcept -> std::string_view {
                                     switch (app_type) {
                                         case message::AppMessageType::kCreateParty:
                                             return kCreateLobby;
                                         case message::AppMessageType::kListParties:
                                             return kListLobby;
                                         case message::AppMessageType::kJoinParty:
                                             return kJoinLobby;
                                         case message::AppMessageType::kStartGame:
                                             return kStartGame;
                                         default:
                                             return kUnknown;
                                     }
                                 },
                                 [](message::DomainMessageType domain_type) noexcept -> std::string_view {
                                     switch (domain_type) {
                                         case message::DomainMessageType::kMove:
                                             return kMove;
                                         case message::DomainMessageType::kAttack:
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
