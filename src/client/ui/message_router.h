#pragma once

#include <common/message.h>

#include "console_output.h"

namespace dungeons::client::ui {

void routeMessage(::network::Message data, const ConsoleOutput& output);

}  // namespace dungeons::client::ui
