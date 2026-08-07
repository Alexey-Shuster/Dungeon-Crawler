#pragma once

#include <functional>
#include <memory>
#include <string>

#include "command_router.h"
#include "message.h"

namespace network {

class Client;

class ClientController : public std::enable_shared_from_this<ClientController> {
public:
    static std::shared_ptr<ClientController> create(std::shared_ptr<Client> client, ConsoleOutput output);

    static std::shared_ptr<ClientController> create(std::shared_ptr<Client> client);
    // Called by Client when a complete frame arrives.
    void onMessageReceived(const MessageData& data) const;

    void onKeyPressed(char key);
    void onStdinLine(const std::string& line);

protected:
    ClientController(std::shared_ptr<network::Client> client, ConsoleOutput output);

private:
    bool sendCommand(const std::string& line) const;

    std::shared_ptr<Client> client_;
    ConsoleOutput output_;
};

}  // namespace network
