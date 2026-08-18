#pragma once

#include <cstdint>
#include <variant>

namespace dungeons::common::types {

enum class Level : uint8_t {
    kNetwork = 0,
    kApp = 1,
    kDomain = 2,
    kUnknown = 3  // for validation / fallback
};

enum class NetworkMessageType : uint8_t {
    kPing = 0,
    kPong = 1,
    kJoin = 2,
    kWelcome = 3,
    kAuthFailed = 4,
    kReconnect = 5,
    kReconnected = 6,
    kNotReconnected = 7,
    kMax  // sentinel, not a real message
};

enum class AppMessageType : uint8_t {
    kCreateParty = 0,
    kPartyCreated = 1,
    kPartyNotCreated = 2,
    kListParties = 3,
    kListPartiesCreated = 4,
    kListPartiesNotCreated = 5,
    kJoinParty = 6,
    kPlayerJoinedParty = 7,
    kPlayerNotJoinedParty = 8,
    kLeaveParty = 9,
    kPlayerLeavedParty = 10,
    kPlayerNotLeavedParty = 11,
    kStartGame = 12,
    kGameStarted = 13,
    kGameNotStarted = 14,
    kPlayerNotJoinedFullParty = 15,
    kPlayerNotJoinedNotExistParty = 16,
    kMax
};

enum class DomainMessageType : uint8_t {
    kMove = 0,
    kPlayerMoved = 1,
    kPlayerNotMoved = 2,
    kAttack = 3,
    kPlayerAttacked = 4,
    kPlayerNotAttacked = 5,
    kStateUpdate = 6,
    kStateNotUpdated = 7,
    kGameOver = 8,
    kMax
};

static_assert(static_cast<uint8_t>(NetworkMessageType::kMax) <= UINT8_MAX, "NetworkMessageType exceeds uint8_t");
static_assert(static_cast<uint8_t>(AppMessageType::kMax) <= UINT8_MAX, "AppMessageType exceeds uint8_t");
static_assert(static_cast<uint8_t>(DomainMessageType::kMax) <= UINT8_MAX, "DomainMessageType exceeds uint8_t");

// Type‑safe variant for convenient handling
using MessageTypeVariant = std::variant<std::monostate,  // unknown / invalid
                                        NetworkMessageType,
                                        AppMessageType,
                                        DomainMessageType>;

}  // namespace dungeons::common::types

namespace dungeons {

using dc_MsgVariant = common::types::MessageTypeVariant;
using dc_NetMsg = common::types::NetworkMessageType;
using dc_AppMsg = common::types::AppMessageType;
using dc_DmnMsg = common::types::DomainMessageType;

}  // namespace dungeons
