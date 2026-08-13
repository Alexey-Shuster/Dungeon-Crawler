#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <deque>
#include <memory>

#include "core/event_bus.h"
#include "raw_message.h"
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

    ~Session();

    void start();

    /**
     * @brief Safely sends a message to the client from any thread.
     * @note The message is encoded into a frame before queueing.
     */
    virtual void send(RawMessageData&& raw_message);

    SessionId getSessionId() const;

    /**
     * @brief Initiates a graceful shutdown of the session.
     * @details Publishes the disconnect event exactly once, even under concurrent calls.
     */
    void handleDisconnect();

protected:
    Session(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid);

private:
    void doRead();

    void doWrite();

    void processMessage(RawMessageData&& raw_msg) const;

private:
    boost::asio::ip::tcp::socket socket_;                       ///< Connection socket (runs in its own io_context)
    boost::asio::strand<boost::asio::any_io_executor> strand_;  ///< Serialises all read/write operations
    core::EventBus& eventBus_;
    const SessionId sessionId_;

    boost::asio::streambuf read_buffer_{};

    // Synchronisation for multi‑threaded environment
    // All access to write_queue_ is performed only on strand_, so no mutex is required
    std::deque<std::shared_ptr<RawMessageData>> write_queue_;  ///< Outgoing message queue
    std::atomic<bool> is_disconnected_{false};  ///< true after session is closed (ensures single event publication)
};

}  // namespace dungeons::server::network
