#pragma once

#include <functional>
#include <string_view>

namespace dungeons::client::ui {

using ConsoleOutput = std::function<void(std::string_view)>;

} // namespace dungeons::client::ui
