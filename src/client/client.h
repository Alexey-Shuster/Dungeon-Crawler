#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "../server/network/raw_message.h"

namespace network {

class ClientManager;

class Client : public std::enable_shared_from_this<Client> {
public:
    static std::shared_ptr<Client> create(boost::asio::io_context& io);

    void startConnect(const std::string& host, uint16_t port);

    bool isConnected() const;

    void send(MessageData message);

    using ConnectionCallback = std::function<void()>;
    using DisconnectionCallback = std::function<void()>;
    using ReceiveMessageCallback = std::function<void(const MessageData&)>;

    void setOnConnect(ConnectionCallback cb);
    void setOnDisconnect(DisconnectionCallback cb);
    void setOnReceiveMessage(ReceiveMessageCallback cb);

private:
    explicit Client(boost::asio::io_context& io);

    void doRead();

    void doWrite();

    void resetState();

    void closeSocket();

    void cleanupSocket();

    void handleDisconnect();

    void doStartConnect(const std::string& host, uint16_t port);

    using tcp = boost::asio::ip::tcp;

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    tcp::resolver resolver_;
    tcp::socket socket_;

    // Incoming data buffer
    boost::asio::streambuf read_buffer_{};

    // Outgoing message queue (encoded frames)
    std::deque<MessageData> write_queue_;

    // Current message being written (kept alive during async_write)
    MessageData current_write_message_{};

    // Prevents overlapping writes
    bool writing_ = false;

    enum class ConnectionState { Disconnected, Connecting, Connected };

    std::atomic<ConnectionState> state_{ConnectionState::Disconnected};

private:
    friend class ClientManager;  // allows manager to set callbacks

    ConnectionCallback on_connect_;
    DisconnectionCallback on_disconnect_;
    ReceiveMessageCallback on_message_;
};

}  // namespace network
