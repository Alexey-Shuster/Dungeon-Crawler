#include "session.h"

#include <common/network/frame_codec.h>
#include <common/types/strong_id_format.h>
#include <common/utility/logger.h>

#include "events.h"

namespace {
static constexpr size_t kBufferChunkSize = 16 * 1024;
}

namespace dungeons::server::network {

std::shared_ptr<Session> Session::create(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid) {
    struct CreateMakeShared final : Session {
        CreateMakeShared(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid)
            : Session(std::move(socket), eventBus, sid) {}
    };
    return std::make_shared<CreateMakeShared>(std::move(socket), eventBus, sid);
}

Session::~Session() {
    LOG_INFO(std::format("[Session {}] destroyed", sessionId_));
}

void Session::start() {
    // Initiate read operation on the strand for consistent serialisation
    boost::asio::post(strand_, [self = shared_from_this()]() {
        self->doRead();
    });
    eventBus_.publish(ClientConnectedEvent{shared_from_this()});
}

void Session::send(common::network::RawMessage raw_message) {
    boost::asio::dispatch(strand_, [this, self = shared_from_this(), msg = std::move(raw_message)]() {
        if (state_.load() != State::Connected)
            return;
        // Encode the message into a frame
        auto encoded = common::network::FrameCodec::encodeFrame(std::move(msg.buffer));
        if (encoded.empty()) {
            LOG_ERROR(std::format("[Session {}] encoding failed", sessionId_));
            return;
        }

        auto msg_ptr = std::make_shared<common::network::RawMessage>(std::move(encoded));
        const bool idle = write_queue_.empty();
        write_queue_.push_back(std::move(msg_ptr));
        if (idle) {
            doWrite();  // Already on strand, safe to call directly
        }
    });
}

SessionId Session::getSessionId() const {
    return sessionId_;
}

void Session::disconnect() {
    boost::asio::post(strand_, [self = shared_from_this()]() {
        self->doDisconnect();
    });
}

Session::Session(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid)
    : socket_(std::move(socket))
    , strand_(socket_.get_executor())
    , eventBus_(eventBus)
    , sessionId_(sid) {
    LOG_INFO(std::format("[Session {}] created", sessionId_));
}

void Session::doRead() {
    if (!socket_.is_open()) {
        LOG_ERROR("Closed socket");
        return;
    }
    // Prepare space in the streambuf
    auto buf = read_buffer_.prepare(kBufferChunkSize);

    socket_.async_read_some(
        buf,
        boost::asio::bind_executor(
            strand_,
            [this, self = shared_from_this()](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    read_buffer_.commit(length);

                    // Extract all complete frames from the accumulated data
                    auto messages = common::network::FrameCodec::extractFrames(read_buffer_);
                    // TODO: update with FrameCodec return logic
                    if (messages.empty()) {
                        LOG_INFO(std::format("[Session {}] no data extracted", sessionId_));
                    } else {
                        LOG_INFO(std::format("[Session {}] read buffer success. Processing total={} messages.",
                                             sessionId_,
                                             messages.size()));
                    }

                    // Process each complete payload
                    int n = 0;
                    for (auto& msg : messages) {
                        ++n;
                        LOG_INFO(std::format("[Session {}] processing message #{}, size={} bytes",
                                             sessionId_,
                                             n,
                                             msg.size()));
                        processMessage(common::network::RawMessage(std::move(msg)));
                        ++n;
                    }

                    // Continue reading immediately
                    doRead();
                } else {
                    LOG_ERROR(std::format("[Session {}] read error: {}", sessionId_, ec.message()));
                    doDisconnect();
                }
            }));
}

void Session::doWrite() {
    if (!socket_.is_open()) {
        LOG_ERROR("Closed socket");
        return;
    }
    if (write_queue_.empty())
        return;

    // Take ownership of the message into a shared_ptr to guarantee the buffer
    // stays alive for the entire asynchronous operation (avoids SSO dangling)
    auto msg_ptr = std::move(write_queue_.front());
    write_queue_.pop_front();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer((*msg_ptr).buffer),
        boost::asio::bind_executor(
            strand_,
            [this, self = shared_from_this(), msg_ptr](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Write succeeded; continue with next queued message (if any)
                    if (!write_queue_.empty()) {
                        doWrite();
                    }
                } else {
                    LOG_ERROR(std::format("[Session {}] write error: {}", sessionId_, ec.message()));
                    doDisconnect();
                }
            }));
}

void Session::processMessage(common::network::RawMessage raw_msg) const {
    LOG_INFO(std::format("[Session {}] sending RawMessageReceivedEvent. Included message size={} bytes",
                         sessionId_,
                         raw_msg.buffer.size()));
    eventBus_.publish(RawMessageReceivedEvent{getSessionId(), std::move(raw_msg)});
}

void Session::doDisconnect() {
    if (state_.exchange(State::Disconnected) == State::Disconnected)
        return;  // already disconnected

    // Now safe to clear queue and close socket (no concurrent I/O)
    write_queue_.clear();
    boost::system::error_code ec;
    socket_.shutdown(socket_.shutdown_both, ec);
    socket_.close(ec);

    eventBus_.publish(ClientDisconnectedEvent{shared_from_this()});
}

bool Session::isConnected() const {
    return state_.load() == State::Connected;
}

}  // namespace dungeons::server::network
