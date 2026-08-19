#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <common/network/raw_message.h>
#include <deque>
#include <memory>
#include <server/core/event_bus.h>

#include "events.h"
#include "types.h"

namespace dungeons::server::network {

/**
 * @brief Represents an active network connection to a client.
 *
 * Thread‑safe class that manages asynchronous read/write via boost::asio.
 * Publishes connect and disconnect events through the central EventBus.
 * Uses shared_ptr for automatic lifetime management.
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    static std::shared_ptr<Session> create(boost::asio::ip::tcp::socket socket,
                                           core::EventBus& eventBus,
                                           SessionId sid);

    virtual ~Session();

    void start();
    virtual void send(common::network::RawMessage raw_message);
    SessionId getSessionId() const;
    bool isConnected() const;
    void disconnect();

protected:
    Session(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid);

private:
    void doRead();
    void doWrite();
    void processMessage(common::network::RawMessage raw_msg) const;
    void doDisconnect();

private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    core::EventBus& eventBus_;
    const SessionId sessionId_;

    boost::asio::streambuf read_buffer_{};

    std::deque<std::shared_ptr<common::network::RawMessage>> write_queue_;

    enum class State {
        Connected,
        Disconnected
    };
    std::atomic<State> state_{State::Connected};
};

}  // namespace dungeons::server::network
