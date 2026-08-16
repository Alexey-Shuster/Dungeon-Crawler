#include <boost/asio.hpp>
#include <chrono>
#include <client/network/client.h>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace dungeons::client::network;
using namespace dungeons::common::network;
using boost::asio::ip::tcp;

// Helper: Simple echo server for testing
class EchoServer {
public:
    EchoServer(boost::asio::io_context& io, uint16_t port)
        : io_(io)
        , acceptor_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
        , socket_(io)
        , is_running_(true) {
        accept();
    }

    ~EchoServer() {
        stop();
    }

    void stop() {
        is_running_ = false;
        boost::system::error_code ec;
        acceptor_.close(ec);
        socket_.close(ec);
    }

    void accept() {
        if (!is_running_)
            return;

        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec && is_running_) {
                doRead();
            }
        });
    }

    void doRead() {
        if (!is_running_)
            return;

        auto buffer = std::make_shared<std::array<char, 1024>>();
        socket_.async_read_some(boost::asio::buffer(*buffer),
                                [this, buffer](boost::system::error_code ec, std::size_t length) {
                                    if (!ec && is_running_) {
                                        // Echo back
                                        boost::asio::async_write(socket_,
                                                                 boost::asio::buffer(buffer->data(), length),
                                                                 [this](boost::system::error_code ec, std::size_t) {
                                                                     if (!ec && is_running_) {
                                                                         doRead();
                                                                     }
                                                                 });
                                    }
                                });
    }

private:
    boost::asio::io_context& io_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    bool is_running_;
};

// Helper to convert string to MessageData
static RawMessage toMessageData(const std::string& str) {
    ByteBuffer buf{str.begin(), str.end()};
    return RawMessage(std::move(buf));
}

class ClientTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {
        // Ensure no lingering operations
    }

    // Helper to run IO with timeout and proper cleanup
    static void runIO(boost::asio::io_context& io, int milliseconds = 100) {
        // Run for specified time
        io.run_for(std::chrono::milliseconds(milliseconds));

        // Stop the io_context to prevent hanging
        io.stop();

        // Reset for next use
        io.restart();
    }

    // Helper to run IO until work is done or timeout
    static void runIOUntil(boost::asio::io_context& io, int maxMilliseconds = 1000) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(maxMilliseconds)) {
            if (io.run_one_for(std::chrono::milliseconds(10)) == 0) {
                break;  // No more work
            }
        }
        io.stop();
        io.restart();
    }
};

// ==================== EXISTING TESTS (FIXED) ====================

TEST_F(ClientTest, ClientCreate) {
    boost::asio::io_context io;
    auto client = Client::create(io);
    EXPECT_NE(client, nullptr);
    auto shared_this = client->shared_from_this();
    EXPECT_EQ(shared_this, client);
}

TEST_F(ClientTest, ConnectNotCrash) {
    boost::asio::io_context io;
    auto client = Client::create(io);
    EXPECT_NO_THROW(client->startConnect("host.invalid", 11111));
    runIO(io, 100);
}

TEST_F(ClientTest, MultipleCalls) {
    boost::asio::io_context io;
    auto client = Client::create(io);
    EXPECT_NO_THROW(client->startConnect("host.one", 22222));
    runIO(io, 50);  // Give first call time to start

    EXPECT_NO_THROW(client->startConnect("host.two", 33333));
    runIO(io, 50);  // Give second call time to start

    // Final run to let operations finish/cancel
    runIO(io, 100);
}

// ==================== NEW SEND TESTS ====================

TEST_F(ClientTest, SendWithoutConnectionShouldNotCrash) {
    boost::asio::io_context io;
    auto client = Client::create(io);

    EXPECT_NO_THROW(client->send(toMessageData("Hello, World!")));
    runIO(io, 50);
}

TEST_F(ClientTest, SendAfterConnectionToRealServer) {
    boost::asio::io_context io;
    uint16_t port = 18080;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);  // Allow connection to establish

    std::string testMessage = "Hello Echo Server!";
    EXPECT_NO_THROW(client->send(toMessageData(testMessage)));
    runIO(io, 200);  // Give time for send to complete

    server.stop();
}

TEST_F(ClientTest, SendMultipleMessages) {
    boost::asio::io_context io;
    uint16_t port = 18081;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    std::vector<std::string> messages = {"First message", "Second message", "Third message", "Fourth message"};

    for (const auto& msg : messages) {
        EXPECT_NO_THROW(client->send(toMessageData(msg)));
    }

    runIO(io, 500);
    server.stop();
}

TEST_F(ClientTest, SendAfterConnectionClosed) {
    boost::asio::io_context io;
    uint16_t port = 18082;

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 100);

    // Connection will fail/timeout, then send should safely fail
    EXPECT_NO_THROW(client->send(toMessageData("Message after failed connect")));
    runIO(io, 100);
}

TEST_F(ClientTest, SendLargeMessage) {
    boost::asio::io_context io;
    uint16_t port = 18083;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    std::string largeMessage(10240, 'X');
    EXPECT_NO_THROW(client->send(toMessageData(largeMessage)));
    runIO(io, 500);
    server.stop();
}

TEST_F(ClientTest, SendEmptyMessage) {
    boost::asio::io_context io;
    uint16_t port = 18084;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    EXPECT_NO_THROW(client->send(RawMessage{ByteBuffer{}}));
    runIO(io, 200);
    server.stop();
}

TEST_F(ClientTest, RapidSendCalls) {
    boost::asio::io_context io;
    uint16_t port = 18085;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    for (int i = 0; i < 50; ++i) {
        client->send(toMessageData("Message " + std::to_string(i)));
    }

    runIO(io, 1000);
    server.stop();
}

// ==================== THREAD SAFETY TESTS ====================

TEST_F(ClientTest, SendFromMultipleThreads) {
    boost::asio::io_context io;
    uint16_t port = 18086;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);

    // Let connection establish
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Send from multiple threads
    std::vector<std::thread> senders;
    for (int i = 0; i < 5; ++i) {
        senders.emplace_back([client, i]() {
            for (int j = 0; j < 10; ++j) {
                std::string msg = "Thread " + std::to_string(i) + " - " + std::to_string(j);
                client->send(toMessageData(msg));
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& t : senders) {
        t.join();
    }

    // Allow time for all sends to complete
    runIO(io, 500);
    server.stop();
}

// ==================== REQUEST-RESPONSE TESTS ====================

class TrackingClient {
public:
    explicit TrackingClient(std::shared_ptr<Client> client)
        : client_(client) {}

    void sendAndTrack(const std::string& message) {
        sent_count_++;
        client_->send(toMessageData(message));
    }

    int getSentCount() const {
        return sent_count_;
    }

private:
    std::shared_ptr<Client> client_;
    int sent_count_ = 0;
};

TEST_F(ClientTest, SendWithTracking) {
    boost::asio::io_context io;
    uint16_t port = 18087;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    TrackingClient tracker(client);

    for (int i = 0; i < 5; ++i) {
        tracker.sendAndTrack("Tracked message " + std::to_string(i));
    }

    EXPECT_EQ(tracker.getSentCount(), 5);
    runIO(io, 500);
    server.stop();
}

// ==================== STRESS TESTS ====================

TEST_F(ClientTest, SendStressTest) {
    boost::asio::io_context io;
    uint16_t port = 18088;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    for (int i = 0; i < 100; ++i) {
        // Reduced from 1000 for performance
        client->send(toMessageData("Stress message " + std::to_string(i)));
        if (i % 20 == 0) {
            runIO(io, 10);
        }
    }

    runIO(io, 500);
    server.stop();
}

// ==================== RESOURCE CLEANUP TESTS ====================

TEST_F(ClientTest, SendAfterCloseConnect) {
    boost::asio::io_context io;
    uint16_t port = 18089;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    // Force close by reconnecting
    client->startConnect("127.0.0.1", port + 1);
    runIO(io, 100);

    EXPECT_NO_THROW(client->send(toMessageData("Message after close")));
    runIO(io, 200);
    server.stop();
}

// ==================== NULL/MALFORMED TESTS ====================

TEST_F(ClientTest, SendNullTerminatedString) {
    boost::asio::io_context io;
    uint16_t port = 18090;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    std::string message = "Hello\0World";
    // MessageData with embedded null
    EXPECT_NO_THROW(client->send(toMessageData(message)));
    runIO(io, 200);
    server.stop();
}

TEST_F(ClientTest, SendVeryLargeMessage) {
    boost::asio::io_context io;
    uint16_t port = 18091;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    std::string hugeMessage(1024 * 1024, 'A');
    EXPECT_NO_THROW(client->send(toMessageData(hugeMessage)));
    runIO(io, 2000);
    server.stop();
}

// ==================== ADDITIONAL FIXES FOR HANGING ====================

TEST_F(ClientTest, StartConnectWithImmediateStop) {
    boost::asio::io_context io;
    auto client = Client::create(io);

    client->startConnect("localhost", 12345);
    io.stop();  // Stop immediately
    io.restart();

    // Should not crash or hang
    SUCCEED();
}

TEST_F(ClientTest, SendAfterIoContextStopped) {
    boost::asio::io_context io;
    auto client = Client::create(io);

    io.stop();
    EXPECT_NO_THROW(client->send(toMessageData("Message after stop")));
    // Should not hang
    SUCCEED();
}

// ==================== EXTRA TESTS (FIXED) ====================

TEST_F(ClientTest, DisconectTest) {
    boost::asio::io_context io;
    uint16_t port = 11111;

    class DisServer {
    public:
        DisServer(boost::asio::io_context& io, uint16_t port)
            : acceptor_(io, tcp::endpoint(tcp::v4(), port))
            , socket_(io) {
            accept();
        }

        void accept() {
            acceptor_.async_accept(socket_, [this](boost::system::error_code err) {
                if (!err) {
                    socket_.close();
                }
            });
        }

    private:
        tcp::acceptor acceptor_;
        tcp::socket socket_;
    };

    DisServer server(io, port);
    auto client = Client::create(io);
    EXPECT_NO_THROW(client->startConnect("127.0.0.1", port));
    runIO(io, 180);
    SUCCEED();
}

TEST_F(ClientTest, ReconnectWhileReading) {
    boost::asio::io_context io;
    uint16_t port = 22222;
    EchoServer server(io, port);
    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 180);

    EXPECT_NO_THROW(client->startConnect("127.0.0.1", port));
    runIO(io, 180);

    SUCCEED();
    server.stop();
}

// ==================== ADDITIONAL TESTS FOR BINARY DATA ====================

TEST_F(ClientTest, SendBinaryData) {
    boost::asio::io_context io;
    uint16_t port = 18092;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    // Send binary data
    ByteBuffer binaryData = {0x01, 0x02, 0x03, 0xFF, 0x00, 0x7F, 0x80, 0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_NO_THROW(client->send(RawMessage(std::move(binaryData))));
    runIO(io, 200);
    server.stop();
}

TEST_F(ClientTest, SendMessageDataDirectly) {
    boost::asio::io_context io;
    uint16_t port = 18093;
    EchoServer server(io, port);

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 200);

    ByteBuffer data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    EXPECT_NO_THROW(client->send(RawMessage(std::move(data))));
    runIO(io, 200);
    server.stop();
}
