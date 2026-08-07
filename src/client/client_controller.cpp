#include "client_controller.h"

#include <format>
#include <iostream>

#include "client.h"
#include "keyboard_controller.h"
#include "message_router.h"

namespace network {

std::shared_ptr<ClientController> ClientController::create(std::shared_ptr<Client> client, ConsoleOutput output) {
    if (!client) {
        throw std::invalid_argument("ClientMessageDispatcher::create: client cannot be null");
    }

    struct EnableMakeShared : ClientController {
        EnableMakeShared(std::shared_ptr<Client> clt, ConsoleOutput out) :
            ClientController(std::move(clt), std::move(out)) {}
    };

    return std::make_shared<EnableMakeShared>(std::move(client), std::move(output));
}

// overload with a default output
std::shared_ptr<ClientController> ClientController::create(std::shared_ptr<Client> client) {
    return create(std::move(client), [](std::string_view s) {
        std::cout << s << std::endl;
    });
}

ClientController::ClientController(std::shared_ptr<Client> client, ConsoleOutput output) :
    client_(std::move(client)), output_(std::move(output)) {}

bool ClientController::sendCommand(const std::string& line) const {
    auto maybeMsg = command_router::route(line, output_);
    if (!maybeMsg)
        return false;

    client_->send(std::move(*maybeMsg));

    return true;
}

void ClientController::onMessageReceived(const MessageData& data) const {
    message_router::route(data, output_);
}

void ClientController::onKeyPressed(char key) {
    if (auto cmd = keyboard_controller::commandFromKey(key)) {
        sendCommand(*cmd);
    }
}

void ClientController::onStdinLine(const std::string& line) {
    if (!sendCommand(line)) {
        output_("Failed to send command");
    }
}

}  // namespace network
