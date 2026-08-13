#pragma once

#include <common/strong_id.h>

namespace dungeons::server::network {

struct SessionIdTag {};

using SessionId = common::StrongId<SessionIdTag>;

using SessionHash = common::StrongIdIdentityHash<SessionId>;

}  // namespace dungeons::server::network
