#include <boost/asio/io_context.hpp>
#include <client/app/client_controller.h>
#include <client/network/client.h>
#include <common/utility/config.h>
#include <common/utility/keyboard_reader.h>
#include <common/utility/logger.h>
#include <common/utility/stdin_reader.h>
#include <common/utility/terminal_guard.h>
#include <exception>
#include <format>
#include <iostream>

using namespace dungeons::client;
using namespace dungeons::common;

int main() {
    try {
        utility::Settings::initialize("config.json");
        const auto& cfg = utility::getSettings();
        utility::Logger::Initialize({cfg.logger.output_dir, cfg.logger.level, cfg.logger.format});

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

        auto client = dungeons::client::network::Client::create(io_context);
        auto client_controller = app::ClientController::create(client);

        auto terminal_guard = std::make_shared<utility::TerminalGuard>();
        auto stdin_reader = std::make_shared<utility::StdinReader>(io_context);
        auto keyboard_reader = std::make_shared<utility::KeyboardReader>(io_context);

        signals.async_wait(
            [&io_context, terminal_guard, stdin_reader, keyboard_reader](const boost::system::error_code& ec,
                                                                         [[maybe_unused]] int signal_number) {
                if (!ec) {
                    LOG_INFO(std::format("Client interrupted by signal: {}", signal_number));

                    terminal_guard->restore();
                    stdin_reader->stop();
                    keyboard_reader->stop();
                    io_context.stop();
                }
            });

        // Connect callback: forward incoming frames to dispatcher
        client->setOnReceiveMessage([client_controller](auto data) {
            client_controller->onMessageReceived(std::move(data));
        });

        // Stdin reader (manual commands)
        stdin_reader->start([client_controller](const std::string& line) {
            client_controller->onStdinLine(line);
        });

        // Keyboard reader (real‑time keyboard commands)
        keyboard_reader->start(
            [client_controller](char key) {
                client_controller->onKeyPressed(key);
            },
            cfg.server.tick_rate);

        client->startConnect(cfg.server.host, cfg.server.port);

        std::cout << "Terminal input disabled.\n"
                  << "Press num-keys to connect and start game.\n"
                  << "1 - Join server, 2 - Create party, 3 - Join party, 4 - Set ready to play, 5 - Start game, 6 - "
                     "Reconnect, 7 - Exit.\n"
                  << "Player control: WASD for movement, X - attack.\n";

        auto log_str = std::format("Client started. Connecting to {}:{}", cfg.server.host, cfg.server.port);
        LOG_INFO(log_str);
        std::cout << log_str << std::endl;

        io_context.run();

    } catch (const std::exception& e) {
        LOG_ERROR(std::format("Client stopped with exception: {}", e.what()));
        return EXIT_FAILURE;
    }

    std::string stop_str = "Client stopped";
    LOG_INFO(stop_str);
    std::cout << stop_str << std::endl;

    utility::Logger::Reset();

    return EXIT_SUCCESS;
}
