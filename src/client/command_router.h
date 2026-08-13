#pragma once

#include <optional>
#include <string>

#include "../server/network/raw_message.h"
#include "console_output.h"

namespace network {

class Client;

namespace command_router {

std::optional<MessageData> route(const std::string& line, const ConsoleOutput& output);

}  // namespace command_router

}  // namespace network
