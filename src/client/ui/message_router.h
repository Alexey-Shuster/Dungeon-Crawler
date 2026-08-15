#pragma once

#include <common/network/raw_message.h>

#include "console_output.h"

namespace dungeons::client::ui {

void routeMessage(common::network::RawMessage data, const ConsoleOutput& output);

}  // namespace dungeons::client::ui
