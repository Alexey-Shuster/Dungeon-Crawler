#pragma once

#include <common/types/strong_id.h>

namespace dungeons::server::domain {

struct PlayerIdTag {};
struct MobIdTag {};
struct LobbyIdTag {};
struct PartyIdTag {};
struct GameIdTag {};
struct EntityIdTag {};

using PlayerId = common::types::StrongId<PlayerIdTag>;
using MobId = common::types::StrongId<MobIdTag>;
using LobbyId = common::types::StrongId<LobbyIdTag>;
using PartyId = common::types::StrongId<PartyIdTag>;
using GameId = common::types::StrongId<GameIdTag>;
using EntityId = common::types::StrongId<EntityIdTag>;

using PlayerHash = common::types::StrongIdIdentityHash<domain::PlayerId>;
using MobHash = common::types::StrongIdIdentityHash<MobId>;
using LobbyHash = common::types::StrongIdIdentityHash<LobbyId>;
using PartyHash = common::types::StrongIdIdentityHash<PartyId>;
using GameHash = common::types::StrongIdIdentityHash<GameId>;
using EntityHash = common::types::StrongIdIdentityHash<EntityId>;

}  // namespace dungeons::server::domain
