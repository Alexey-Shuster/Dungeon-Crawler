#pragma once

#include <optional>
#include <string>

#include "console_output.h"
#include "message.h"

namespace network {

class Client;

namespace command_router {

std::optional<MessageData> route(const std::string& line, const ConsoleOutput& output);

}  // namespace command_router

}  // namespace network
