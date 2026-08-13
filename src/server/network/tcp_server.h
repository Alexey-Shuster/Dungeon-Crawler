#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <stdint.h>

#include "core/event_bus.h"

namespace dungeons::server::network {

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    TcpServer(boost::asio::io_context& io, std::shared_ptr<core::EventBus> event_bus);

    void start();

private:
    void doAccept();

private:
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<core::EventBus> event_bus_;
    std::atomic<uint64_t> next_session_id_{1};
};
}  // namespace dungeons::server::network
