#pragma once

#include <common/message.h>
#include <memory>
#include <string>

#include "network/client_fwd.h"
#include "ui/command_router.h"

namespace dungeons::client::app {

class ClientController : public std::enable_shared_from_this<ClientController> {
public:
    static std::shared_ptr<ClientController> create(std::shared_ptr<network::Client> client, ui::ConsoleOutput output);
    static std::shared_ptr<ClientController> create(std::shared_ptr<network::Client> client);

    void onMessageReceived(::network::Message data) const;
    void onKeyPressed(char key);
    void onStdinLine(const std::string& line);

protected:
    ClientController(std::shared_ptr<network::Client> client, ui::ConsoleOutput output);

private:
    bool sendCommand(const std::string& line) const;

    std::shared_ptr<network::Client> client_;
    ui::ConsoleOutput output_;
};

}  // namespace dungeons::client::app
