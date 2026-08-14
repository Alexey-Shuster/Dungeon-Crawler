#include "keyboard_controller.h"

namespace dungeons::client::ui {

// TODO: handle all commands
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
        case '1':
            return "JOIN 12345";
        case '2':
            return "CREATE_LOBBY";
        case '3':
            return "";
        case '4':
            return "";
        case '5':
            return "START_GAME";
        case '6':
            return "";
        case '7':
            return "EXIT";

        default:
            return std::nullopt;
    }
}

}  // namespace dungeons::client::ui
