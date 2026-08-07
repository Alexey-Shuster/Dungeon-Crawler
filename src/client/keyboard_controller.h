#pragma once

#include <optional>
#include <string>

namespace network::keyboard_controller {

[[nodiscard]] std::optional<std::string> commandFromKey(char key);

}
