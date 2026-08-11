#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <csignal>
#include <format>
#include <iostream>
#include <memory>
#include <thread>

#include "../app/game_manager.h"
#include "../domain/game/dungeon_registry.h"
#include "config.h"
#include "connection_manager.h"
#include "eventbus.h"
#include "game_loop.h"
#include "lobby_manager.h"
#include "lobby_registry.h"
#include "logger.h"
#include "message_router.h"
#include "response_sender.h"
#include "session_registry.h"
#include "tcp_server.h"

using namespace boost::asio;

int main(int argc, char* argv[]) {
    try {
        config::Settings::initialize("config.json");
        const auto& cfg = config::getSettings();
        utils::Logger::Initialize({cfg.logger.output_dir, cfg.logger.level, cfg.logger.format});

        LOG_INFO("Starting Dungeon Crawler Server ...");
        auto event_bus = events::EventBus::create();

        auto session_registry = std::make_shared<network::SessionRegistry>();
        auto lobby_registry = std::make_shared<lobby::LobbyRegistry>();
        auto dungeon_registry = std::make_shared<dungeon::DungeonRegistry>();

        io_context io_context;

        auto connection_manager = connection_manager::ConnectionManager::Create(event_bus, session_registry);
        auto game_manager = game::GameManager::create(io_context, event_bus, lobby_registry);
        auto lobby_manager = lobby::LobbyManager::create(io_context, event_bus, lobby_registry);

        auto message_router = message_router::MessageRouter::create(event_bus, session_registry);
        auto response_sender = network::ResponseSender::create(event_bus, session_registry);

        auto server = std::make_shared<network::TcpServer>(io_context, event_bus);
        auto game_loop = std::make_shared<infra::GameLoop>(io_context, event_bus, cfg.server.tick_rate);

        server->start();
        game_loop->start();

        LOG_INFO(std::format("Server started on port {}", cfg.server.port));

        signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& err, int signal_number) {
            if (!err) {
                LOG_INFO(std::format("Signal {} , shutting down.", signal_number));
                io_context.stop();
            }
        });

        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count < 2)
            thread_count = 2;
        LOG_INFO(std::format("Start {} worker threads", thread_count));

        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < thread_count; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        for (auto& th : threads) {
            if (th.joinable())
                th.join();
        }

        LOG_INFO("Server stopped gracefully");

    } catch (const boost::system::system_error& e) {
        auto error_str = std::format("Server initialization failed: {}", e.what());
        LOG_ERROR(error_str);
        std::cout << error_str << std::flush;

        return 1;
    } catch (const std::exception& e) {
        auto error_str = std::format("Unexpected error: {}", e.what());
        LOG_ERROR(error_str);
        std::cout << error_str << std::flush;

        return 1;
    }
    return 0;
}
