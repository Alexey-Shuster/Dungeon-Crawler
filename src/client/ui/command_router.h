#pragma once

#include <common/network/raw_message.h>
#include <optional>
#include <string>

#include "console_output.h"

namespace dungeons::client::ui {

class Client;

std::optional<::network::Message> routeCommand(const std::string& line, const ConsoleOutput& output);

}  // namespace dungeons::client::ui
