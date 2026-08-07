#include "client.h"

#include <format>
#include <string>

#include "../common/frame_codec.h"
#include "../common/logger.h"

namespace {
// Maximum number of pending outgoing messages
constexpr size_t kMaxQueueSize = 1000;

constexpr size_t kBufferChunkSize = 16 * 1024;
}  // namespace

namespace network {

std::shared_ptr<Client> Client::create(boost::asio::io_context& io) {
    return std::shared_ptr<Client>(new Client(io));
}

void Client::startConnect(const std::string& host, uint16_t port) {
    auto self = shared_from_this();

    boost::asio::post(strand_, [this, self, host, port]() {
        cleanupSocket();
        resetState();
        state_.store(ConnectionState::Connecting);
        doStartConnect(host, port);
    });
}

void Client::doStartConnect(const std::string& host, uint16_t port) {
    auto self = shared_from_this();

    resolver_.async_resolve(
        host,
        std::to_string(port),
        boost::asio::bind_executor(
            strand_,
            [this, self](boost::system::error_code err, tcp::resolver::results_type endpoints) {
                if (err) {
                    LOG_ERROR(std::format("[Client] resolve failed: {}", err.message()));
                    handleDisconnect();
                    return;
                }

                boost::asio::async_connect(
                    socket_,
                    endpoints,
                    boost::asio::bind_executor(strand_, [this, self](boost::system::error_code err, tcp::endpoint) {
                        if (!err) {
                            state_.store(ConnectionState::Connected);
                            LOG_INFO(std::format("[Client] connected to endpoint"));

                            if (on_connect_)
                                on_connect_();

                            doRead();
                        } else {
                            LOG_ERROR(std::format("[Client] connection failed: {}", err.message()));
                            handleDisconnect();
                        }
                    }));
            }));
}

bool Client::isConnected() const {
    return state_.load(std::memory_order_acquire) == ConnectionState::Connected;
}

void Client::doRead() {
    if (!isConnected()) {
        return;
    }
    auto buf = read_buffer_.prepare(kBufferChunkSize);
    auto self = shared_from_this();

    socket_.async_read_some(
        buf,
        boost::asio::bind_executor(strand_, [this, self](boost::system::error_code err, std::size_t length) {
            if (!err) {
                read_buffer_.commit(length);

                // Extract complete frames
                auto messages = FrameCodec::extractFrames(read_buffer_);
                for (auto& msg : messages) {
                    // Process raw message – just log
                    LOG_INFO(std::format("[Client] received: {}", std::string(msg.begin(), msg.end())));

                    if (on_message_)
                        on_message_(msg);
                }

                // Continue reading
                doRead();
            } else {
                LOG_ERROR(std::format("[Client] read failed: {}", err.message()));
                handleDisconnect();
            }
        }));
}

void Client::send(MessageData message) {
    boost::asio::post(strand_, [this, message = std::move(message)]() {
        if (!isConnected() || !socket_.is_open()) {
            LOG_ERROR(std::format("[Client] cannot send – socket closed or disconnected"));
            return;
        }

        // Encode the raw message into a frame
        MessageData encoded = FrameCodec::encodeFrame(message);
        if (encoded.empty()) {
            LOG_ERROR(std::format("[Client] encoding failed"));
            return;
        }

        // Enforce queue size limit
        if (write_queue_.size() >= kMaxQueueSize) {
            LOG_ERROR(std::format("[Client] write queue full, dropping message"));
            return;
        }

        write_queue_.push_back(std::move(encoded));

        // Start writing if not already in progress
        if (!writing_) {
            doWrite();
        }
    });
}
void Client::setOnConnect(ConnectionCallback cb) {
    on_connect_ = std::move(cb);
}
void Client::setOnDisconnect(DisconnectionCallback cb) {
    on_disconnect_ = std::move(cb);
}
void Client::setOnReceiveMessage(ReceiveMessageCallback cb) {
    on_message_ = std::move(cb);
}

Client::Client(boost::asio::io_context& io) : strand_(boost::asio::make_strand(io)), resolver_(io), socket_(io) {}

void Client::doWrite() {
    if (write_queue_.empty() || writing_ || !isConnected()) {
        return;
    }

    writing_ = true;
    current_write_message_ = std::move(write_queue_.front());
    write_queue_.pop_front();

    auto self = shared_from_this();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(current_write_message_),
        boost::asio::bind_executor(strand_, [this, self](boost::system::error_code err, std::size_t /*bytes*/) {
            writing_ = false;
            if (!err) {
                LOG_INFO(std::format("[Client] sent bytes: {}", current_write_message_.size()));
                if (!write_queue_.empty()) {
                    doWrite();
                }
            } else {
                LOG_ERROR(std::format("[Client] write failed: {}", err.message()));
                handleDisconnect();
            }
        }));
}

void Client::resetState() {
    write_queue_.clear();
    writing_ = false;
    current_write_message_.clear();
    read_buffer_.consume(read_buffer_.size());
}

void Client::closeSocket() {
    if (!socket_.is_open()) {
        return;
    }

    boost::system::error_code err;
    socket_.close(err);

    if (err) {
        LOG_ERROR(std::format("[Client] socket close failed: {}", err.message()));
    }
}

void Client::cleanupSocket() {
    resolver_.cancel();
    closeSocket();
}

void Client::handleDisconnect() {
    if (state_.exchange(ConnectionState::Disconnected) == ConnectionState::Disconnected) {
        return;  // already handling
    }

    resetState();
    closeSocket();

    LOG_INFO(std::format("[Client] disconnected"));

    if (on_disconnect_)
        on_disconnect_();
}

}  // namespace network
