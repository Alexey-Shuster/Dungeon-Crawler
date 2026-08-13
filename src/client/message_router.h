#pragma once

#include "console_output.h"
#include "network/raw_message.h"

namespace network { namespace message_router {

void route(const MessageData& data, const ConsoleOutput& output);

}}  // namespace network::message_router
