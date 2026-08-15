#pragma once

#include <cbor.h>
#include <network/game_state_dto.h>
#include <optional>

#include "serder_base.h"

namespace dungeons::common::wire::detail {

CborPtr serialize(const network::BarrierSnapshot& barrier);

std::optional<network::BarrierSnapshot> deserialize(cbor_item_t* item, network::BarrierSnapshot*);

CborPtr serialize(const network::EntitySnapshot& entity);

std::optional<network::EntitySnapshot> deserialize(cbor_item_t* item, network::EntitySnapshot*);

CborPtr serialize(const network::GameMapSnapshot& map);

std::optional<network::GameMapSnapshot> deserialize(cbor_item_t* item, network::GameMapSnapshot*);

}  // namespace dungeons::common::wire
