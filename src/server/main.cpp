#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <common/config.h>
#include <common/logger.h>
#include <csignal>
#include <format>
#include <iostream>
#include <memory>
#include <thread>

#include "app/connection_manager.h"
#include "app/game_manager.h"
#include "app/message_router.h"
#include "app/response_sender.h"
#include "app/session_registry.h"
#include "core/event_bus.h"
#include "core/game_loop.h"
#include "domain/dungeon/dungeon_registry.h"
#include "domain/lobby/lobby_manager.h"
#include "domain/lobby/lobby_registry.h"
#include "network/tcp_server.h"

using namespace boost::asio;
using namespace dungeons::server;

int main(int argc, char* argv[]) {
    try {
        config::Settings::initialize("config.json");
        const auto& cfg = config::getSettings();
        utils::Logger::Initialize({cfg.logger.output_dir, cfg.logger.level, cfg.logger.format});

        LOG_INFO("Starting Dungeon Crawler Server ...");
        auto event_bus = core::EventBus::create();

        auto session_registry = std::make_shared<app::SessionRegistry>();
        auto lobby_registry = std::make_shared<domain::LobbyRegistry>();
        auto dungeon_registry = std::make_shared<domain::DungeonRegistry>();

        io_context io_context;

        auto connection_manager = app::ConnectionManager::Create(event_bus, session_registry);
        auto game_manager = app::GameManager::create(io_context, event_bus, lobby_registry);
        auto lobby_manager = domain::LobbyManager::create(io_context, event_bus, lobby_registry);

        auto message_router = app::MessageRouter::create(event_bus, session_registry);
        auto response_sender = app::ResponseSender::create(event_bus, session_registry);

        auto server = std::make_shared<dungeons::server::network::TcpServer>(io_context, event_bus);
        auto game_loop = std::make_shared<core::GameLoop>(io_context, event_bus, cfg.server.tick_rate);

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
