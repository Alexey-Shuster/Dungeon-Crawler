#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <deque>
#include <memory>
#include <vector>

#include "client.h"

namespace network {

struct ClientConfig {
    std::string host;
    int port;
    size_t client_count = 5;

    bool enable_auth = true;
    std::string auth_message = "JOIN Player";
    std::chrono::milliseconds auth_timeout{1000};

    bool enable_actions = true;
    std::string action_message = "MOVE Up";
    std::chrono::milliseconds send_interval{200};
    size_t max_messages_per_client = 0;  // 0 = unlimited

    bool enable_reconnect = false;
    std::chrono::milliseconds reconnect_delay{2000};
};

class ClientManager : public std::enable_shared_from_this<ClientManager> {
public:
    static std::shared_ptr<ClientManager> create(boost::asio::io_context& io, const ClientConfig& config);

    // Start all clients
    void start();

    // Stop all clients (cancel timers, close sockets)
    void stop();

    // Toggle actions globally (stops/starts sending)
    void setActionsEnabled(bool enabled);

    // Toggle authentication for *new* connections (already connected stay as is)
    void setAuthEnabled(bool enabled);

protected:
    ClientManager(boost::asio::io_context& io, const ClientConfig& config);

private:
    struct ManagedClient {
        std::shared_ptr<Client> client;
        std::shared_ptr<boost::asio::steady_timer> action_timer;
        std::shared_ptr<boost::asio::steady_timer> auth_timer;
        enum State { Disconnected, Connecting, Authenticating, Ready } state = Disconnected;
        bool auth_enabled;
        bool actions_enabled;
        size_t messages_sent = 0;
        bool reconnect_pending = false;
    };

    void connectClient(size_t index);
    void onConnected(size_t index);
    void onDisconnected(size_t index);
    void onMessage(size_t index, const MessageData& msg);

    void performAuth(size_t index);
    void onAuthTimeout(size_t index);
    void authResponse(size_t index, const MessageData& response);

    void startSending(size_t index);
    void doSend(size_t index);

    void scheduleReconnect(size_t index);

    boost::asio::io_context& io_;
    ClientConfig config_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::vector<ManagedClient> clients_;
    bool actions_global_enabled_;
    bool auth_global_enabled_;
};

}  // namespace network
