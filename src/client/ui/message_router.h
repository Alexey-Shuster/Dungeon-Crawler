#pragma once

#include <common/network/raw_message.h>

#include "console_output.h"

namespace dungeons::client::ui {

void routeMessage(::network::Message data, const ConsoleOutput& output);

}  // namespace dungeons::client::ui
