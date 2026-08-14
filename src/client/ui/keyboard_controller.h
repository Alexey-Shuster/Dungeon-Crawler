#pragma once

#include <optional>
#include <string>

namespace dungeons::client::ui {

[[nodiscard]] std::optional<std::string> commandFromKey(char key);

}  // namespace dungeons::client::ui
