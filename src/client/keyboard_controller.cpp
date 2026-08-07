#include "keyboard_controller.h"

namespace network::keyboard_controller {

[[nodiscard]] std::optional<std::string> commandFromKey(char key) {
    const char lowerKey = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));

    switch (lowerKey) {
        case 'w':
            return "MOVE UP";
        case 'a':
            return "MOVE LEFT";
        case 's':
            return "MOVE DOWN";
        case 'd':
            return "MOVE RIGHT";
        case 'x':
            return "ATTACK";

        default:
            return std::nullopt;
    }
}

}  // namespace network::keyboard_controller
