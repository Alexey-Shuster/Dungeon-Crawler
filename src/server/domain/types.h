#pragma once

#include <common/strong_id.h>

namespace dungeons::server::domain {

struct PlayerIdTag {};
struct MobIdTag {};
struct LobbyIdTag {};
struct PartyIdTag {};
struct GameIdTag {};
struct EntityIdTag {};

using PlayerId = common::StrongId<PlayerIdTag>;
using MobId = common::StrongId<MobIdTag>;
using LobbyId = common::StrongId<LobbyIdTag>;
using PartyId = common::StrongId<PartyIdTag>;
using GameId = common::StrongId<GameIdTag>;
using EntityId = common::StrongId<EntityIdTag>;

using PlayerHash = common::StrongIdIdentityHash<domain::PlayerId>;
using MobHash = common::StrongIdIdentityHash<MobId>;
using LobbyHash = common::StrongIdIdentityHash<LobbyId>;
using PartyHash = common::StrongIdIdentityHash<PartyId>;
using GameHash = common::StrongIdIdentityHash<GameId>;
using EntityHash = common::StrongIdIdentityHash<EntityId>;

}  // namespace dungeons::server::domain
