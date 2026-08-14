#include "session.h"

#include <common/frame_codec.h>
#include <common/logger.h>
#include <format>

#include "events.h"

namespace {
static constexpr size_t kBufferChunkSize = 16 * 1024;
}

namespace dungeons::server::network {

std::shared_ptr<Session> Session::create(boost::asio::ip::tcp::socket socket,
                                         core::EventBus& eventBus,
                                         SessionId sid) {
    struct CreateMakeShared : Session {
        CreateMakeShared(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid)
            : Session(std::move(socket), eventBus, sid) {}
    };
    return std::make_shared<CreateMakeShared>(std::move(socket), eventBus, sid);
}

Session::~Session() {
    LOG_INFO(std::format("[Session {}] destroyed", sessionId_.value));
}

void Session::start() {
    // Initiate read operation on the strand for consistent serialisation
    boost::asio::post(strand_, [self = shared_from_this()]() {
        self->doRead();
    });
    eventBus_.publish(ClientConnectedEvent{shared_from_this()});
}

void Session::send(::network::Message raw_message) {
    boost::asio::dispatch(strand_, [this, self = shared_from_this(), msg = std::move(raw_message)]() {
        if (is_disconnected_.load(std::memory_order_acquire)) {
            return;
        }
        // Encode the message into a frame
        auto encoded = ::network::FrameCodec::encodeFrame(std::move(msg.buffer));
        if (encoded.empty()) {
            LOG_ERROR(std::format("[Session {}] encoding failed", sessionId_.value));
            return;
        }

        auto msg_ptr = std::make_shared<::network::Message>(std::move(encoded));
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

void Session::handleDisconnect() {
    if (is_disconnected_.exchange(true, std::memory_order_acq_rel)) {
        return;  // Already disconnecting
    }

    boost::system::error_code ec;
    socket_.shutdown(socket_.shutdown_both, ec);
    socket_.close(ec);  // Ignore close errors
    write_queue_.clear();

    eventBus_.publish(ClientDisconnectedEvent{shared_from_this()});
}

Session::Session(boost::asio::ip::tcp::socket socket, core::EventBus& eventBus, SessionId sid)
    : socket_(std::move(socket))
    , strand_(socket_.get_executor())
    , eventBus_(eventBus)
    , sessionId_(sid) {
    LOG_INFO(std::format("[Session {}] created", sessionId_.value));
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
                    auto messages = ::network::FrameCodec::extractFrames(read_buffer_);
                    // TODO: update with FrameCodec return logic
                    if (messages.empty()) {
                        LOG_INFO(std::format("[Session {}] no data extracted", sessionId_.value));
                    } else {
                        LOG_INFO(std::format("[Session {}] read buffer success. Processing total={} messages.",
                                             sessionId_.value,
                                             messages.size()));
                    }

                    // Process each complete payload
                    for (auto& msg : messages) {
                        int n = 1;
                        LOG_INFO(std::format("[Session {}] processing message #{}, size={} bytes",
                                             sessionId_.value,
                                             n,
                                             msg.size()));
                        processMessage(::network::Message(std::move(msg)));
                        ++n;
                    }

                    // Continue reading immediately
                    doRead();
                } else {
                    LOG_ERROR(std::format("[Session {}] read error: {}", sessionId_.value, ec.message()));
                    handleDisconnect();  // Error → close
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
                    LOG_ERROR(std::format("[Session {}] write error: {}", sessionId_.value, ec.message()));
                    handleDisconnect();
                }
            }));
}

void Session::processMessage(::network::Message raw_msg) const {
    LOG_INFO(std::format("[Session {}] sending RawMessageReceivedEvent. Included message size={} bytes",
                         sessionId_.value,
                         raw_msg.buffer.size()));
    eventBus_.publish(RawMessageReceivedEvent{getSessionId(), std::move(raw_msg)});
}
}  // namespace dungeons::server::network
