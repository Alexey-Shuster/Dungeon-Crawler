#pragma once

#include <functional>
#include <string_view>

namespace network {

using ConsoleOutput = std::function<void(std::string_view)>;

}
