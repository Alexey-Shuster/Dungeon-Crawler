#pragma once

#include <common/types/strong_id.h>

namespace dungeons::server::network {

struct SessionIdTag {};

using SessionId = common::types::StrongId<SessionIdTag>;

using SessionHash = common::types::StrongIdIdentityHash<SessionId>;

}  // namespace dungeons::server::network
