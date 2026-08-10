#include <boost/asio.hpp>
#include <csignal>
#include <iostream>

#include "../common/config.h"
#include "../common/logger.h"
#include "client_manager.h"

int main(int argc, char* argv[]) {
    config::Settings::initialize("config.json");
    const auto& cfg = config::getSettings();
    utils::Logger::Instance();

    boost::asio::io_context io;

    // Hardcoded config for MVP
    network::ClientConfig config;
    config.host = cfg.server.host;
    config.port = cfg.server.port;
    config.client_count = 3;
    config.enable_auth = true;
    config.auth_message = "JOIN Player";
    config.enable_actions = true;
    config.action_message = "MOVE Up";
    config.send_interval = std::chrono::milliseconds(500);
    config.max_messages_per_client = 10;
    config.enable_reconnect = true;
    config.reconnect_delay = std::chrono::milliseconds(3000);

    auto manager = network::ClientManager::create(io, config);

    // Graceful shutdown on SIGINT/SIGTERM
    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) {
        std::cout << "[INFO] Stopping manager..." << std::endl;
        manager->stop();
        io.stop();
    });

    manager->start();
    std::cout << "[INFO] Starting manager..." << std::endl;
    io.run();

    std::cout << "[INFO] Exiting..." << std::endl;
    return 0;
}
