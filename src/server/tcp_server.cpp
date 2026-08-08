#include "tcp_server.h"

#include <format>

#include "config.h"
#include "events.h"
#include "logger.h"
#include "session.h"
#include "types.h"

namespace network {

namespace asio = boost::asio;
using boost::asio::ip::tcp;

TcpServer::TcpServer(boost::asio::io_context& io, std::shared_ptr<events::EventBus> event_bus) :
    io_context_(io), acceptor_(io), event_bus_(std::move(event_bus)) {
    auto& cfg = config::getSettings();  // читаем конфиги
    std::string ip = cfg.server.host;
    uint16_t port = cfg.server.port;

    tcp::endpoint endpoint(asio::ip::address::from_string(ip), port);

    acceptor_.open(endpoint.protocol());
#ifndef NDEBUG
    acceptor_.set_option(tcp::acceptor::reuse_address(true));  // В debug не ждем закрытия сокета
#endif
    acceptor_.bind(endpoint);
    acceptor_.listen();
}

void TcpServer::start() {
    const auto& local_endpoint = acceptor_.local_endpoint();
    LOG_INFO(std::format("Server start at {}:{}", local_endpoint.address().to_string(), local_endpoint.port()));
    std::cout << "Server start at " << local_endpoint.address() << ":" << local_endpoint.port() << std::endl;

    doAccept();
}

void TcpServer::doAccept() {
    auto weak_self = weak_from_this();
    acceptor_.async_accept([weak_self](boost::system::error_code err, tcp::socket socket) {
        auto self = weak_self.lock();
        if (!self) {
            return;  // TcpServer уже уничтожен, выходим из лямбда-функции
        }
        if (!err) {
            LOG_INFO("New client connected");
            // Создаём сессию
            const auto sid_value = self->next_session_id_.fetch_add(1);
            auto session = Session::create(std::move(socket), *self->event_bus_, SessionId{sid_value});

            // Запускаем сессию
            session->start();
        } else {
            LOG_ERROR(std::format("Accept error: {}", err.message()));
        }
        self->doAccept();
    });
}
}  // namespace network
