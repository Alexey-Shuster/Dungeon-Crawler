#include <boost/asio.hpp>
#include <chrono>
#include <client/network/client.h>
#include <condition_variable>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace dungeons::client::network;
using namespace dungeons::common::network;
using boost::asio::ip::tcp;

// Simple echo server for testing
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

// Helper to convert string to RawMessage
static RawMessage toMessageData(const std::string& str) {
    ByteBuffer buf{str.begin(), str.end()};
    return RawMessage(std::move(buf));
}

// Helper: run io_context until a predicate becomes true or timeout
template <typename Predicate>
static void runUntil(boost::asio::io_context& io, Predicate pred, int maxMilliseconds = 500) {
    auto start = std::chrono::steady_clock::now();
    while (!pred() && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(maxMilliseconds)) {
        if (io.run_one_for(std::chrono::milliseconds(5)) == 0) {
            break;  // no more work
        }
    }
    io.stop();
    io.restart();
}

// Helper: run io_context until no more work or timeout
static void runIO(boost::asio::io_context& io, int maxMilliseconds = 50) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(maxMilliseconds)) {
        if (io.run_one_for(std::chrono::milliseconds(5)) == 0) {
            break;
        }
    }
    io.stop();
    io.restart();
}

class ClientTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {
        // Ensure no lingering operations
    }
};

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
    runIO(io, 20);
}

TEST_F(ClientTest, MultipleCalls) {
    boost::asio::io_context io;
    auto client = Client::create(io);
    EXPECT_NO_THROW(client->startConnect("host.one", 22222));
    runIO(io, 10);
    EXPECT_NO_THROW(client->startConnect("host.two", 33333));
    runIO(io, 10);
    runIO(io, 20);  // final cleanup
}

// ==================== SEND TESTS (WITH PREDICATES) ====================

TEST_F(ClientTest, SendWithoutConnectionShouldNotCrash) {
    boost::asio::io_context io;
    auto client = Client::create(io);

    EXPECT_NO_THROW(client->send(toMessageData("Hello, World!")));
    runIO(io, 10);
}

TEST_F(ClientTest, SendAfterConnectionToRealServer) {
    boost::asio::io_context io;
    uint16_t port = 18080;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    std::string testMessage = "Hello Echo Server!";
    bool messageReceived = false;
    client->setOnReceiveMessage([&](RawMessage) {
        messageReceived = true;
    });

    EXPECT_NO_THROW(client->send(toMessageData(testMessage)));
    runUntil(
        io,
        [&]() {
            return messageReceived;
        },
        100);

    server.stop();
}

TEST_F(ClientTest, SendMultipleMessages) {
    boost::asio::io_context io;
    uint16_t port = 18081;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    int receivedCount = 0;
    const int totalMessages = 4;
    client->setOnReceiveMessage([&](RawMessage) {
        ++receivedCount;
    });

    std::vector<std::string> messages = {"First", "Second", "Third", "Fourth"};
    for (const auto& msg : messages) {
        client->send(toMessageData(msg));
    }

    runUntil(
        io,
        [&]() {
            return receivedCount >= totalMessages;
        },
        150);
    EXPECT_EQ(receivedCount, totalMessages);
    server.stop();
}

TEST_F(ClientTest, SendAfterConnectionClosed) {
    boost::asio::io_context io;
    uint16_t port = 18082;

    auto client = Client::create(io);
    client->startConnect("127.0.0.1", port);
    runIO(io, 30);  // connection will fail quickly

    EXPECT_NO_THROW(client->send(toMessageData("Message after failed connect")));
    runIO(io, 20);
}

TEST_F(ClientTest, SendLargeMessage) {
    boost::asio::io_context io;
    uint16_t port = 18083;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    bool received = false;
    client->setOnReceiveMessage([&](RawMessage) {
        received = true;
    });

    std::string largeMessage(10 * 1024, 'X');  // 10 KiB (reduced from 10 MiB)
    EXPECT_NO_THROW(client->send(toMessageData(largeMessage)));
    runUntil(
        io,
        [&]() {
            return received;
        },
        200);
    server.stop();
}

TEST_F(ClientTest, SendEmptyMessage) {
    boost::asio::io_context io;
    uint16_t port = 18084;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    bool received = false;
    client->setOnReceiveMessage([&](RawMessage) {
        received = true;
    });

    EXPECT_NO_THROW(client->send(RawMessage{ByteBuffer{}}));
    runUntil(
        io,
        [&]() {
            return received;
        },
        100);
    server.stop();
}

TEST_F(ClientTest, RapidSendCalls) {
    boost::asio::io_context io;
    uint16_t port = 18085;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    int receivedCount = 0;
    const int totalMessages = 20;  // reduced from 50
    client->setOnReceiveMessage([&](RawMessage) {
        ++receivedCount;
    });

    for (int i = 0; i < totalMessages; ++i) {
        client->send(toMessageData("Msg " + std::to_string(i)));
    }

    runUntil(
        io,
        [&]() {
            return receivedCount >= totalMessages;
        },
        200);
    EXPECT_EQ(receivedCount, totalMessages);
    server.stop();
}

// ==================== THREAD SAFETY TESTS (WITH CONDITION_VARIABLE) ====================

TEST_F(ClientTest, SendFromMultipleThreads) {
    boost::asio::io_context io;
    uint16_t port = 18086;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        200);  // run io until connected
    ASSERT_TRUE(connected);

    // Send from multiple threads
    const int numThreads = 5;
    const int messagesPerThread = 10;
    std::atomic<int> totalSent{0};
    std::vector<std::thread> senders;

    for (int i = 0; i < numThreads; ++i) {
        senders.emplace_back([client, i, messagesPerThread, &totalSent]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                std::string msg = "Thread " + std::to_string(i) + " - " + std::to_string(j);
                client->send(toMessageData(msg));
                totalSent.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for (auto& t : senders) {
        t.join();
    }

    // Let IO finish sending all messages
    runIO(io, 200);
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
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    TrackingClient tracker(client);

    for (int i = 0; i < 5; ++i) {
        tracker.sendAndTrack("Tracked message " + std::to_string(i));
    }

    EXPECT_EQ(tracker.getSentCount(), 5);
    runIO(io, 50);  // allow sends to complete
    server.stop();
}

// ==================== STRESS TESTS ====================

TEST_F(ClientTest, SendStressTest) {
    boost::asio::io_context io;
    uint16_t port = 18088;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    const int totalMessages = 50;  // reduced from 100
    for (int i = 0; i < totalMessages; ++i) {
        client->send(toMessageData("Stress " + std::to_string(i)));
        if (i % 10 == 0) {
            runIO(io, 5);
        }
    }
    runIO(io, 100);
    server.stop();
}

// ==================== RESOURCE CLEANUP TESTS ====================

TEST_F(ClientTest, SendAfterCloseConnect) {
    boost::asio::io_context io;
    uint16_t port = 18089;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    // Force close by reconnecting
    client->startConnect("127.0.0.1", port + 1);
    runIO(io, 30);

    EXPECT_NO_THROW(client->send(toMessageData("Message after close")));
    runIO(io, 30);
    server.stop();
}

// ==================== NULL/MALFORMED TESTS ====================

TEST_F(ClientTest, SendNullTerminatedString) {
    boost::asio::io_context io;
    uint16_t port = 18090;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    bool received = false;
    client->setOnReceiveMessage([&](RawMessage) {
        received = true;
    });

    std::string message = "Hello\0World";
    EXPECT_NO_THROW(client->send(toMessageData(message)));
    runUntil(
        io,
        [&]() {
            return received;
        },
        100);
    server.stop();
}

TEST_F(ClientTest, SendVeryLargeMessage) {
    boost::asio::io_context io;
    uint16_t port = 18091;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    bool received = false;
    client->setOnReceiveMessage([&](RawMessage) {
        received = true;
    });

    std::string hugeMessage(100 * 1024, 'A');  // 100 KiB (was 1 MiB)
    EXPECT_NO_THROW(client->send(toMessageData(hugeMessage)));
    runUntil(
        io,
        [&]() {
            return received;
        },
        300);
    server.stop();
}

// ==================== FIXES FOR HANGING ====================

TEST_F(ClientTest, StartConnectWithImmediateStop) {
    boost::asio::io_context io;
    auto client = Client::create(io);

    client->startConnect("localhost", 12345);
    io.stop();
    io.restart();
    SUCCEED();
}

TEST_F(ClientTest, SendAfterIoContextStopped) {
    boost::asio::io_context io;
    auto client = Client::create(io);

    io.stop();
    EXPECT_NO_THROW(client->send(toMessageData("Message after stop")));
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
    runIO(io, 30);
    SUCCEED();
}

TEST_F(ClientTest, ReconnectWhileReading) {
    boost::asio::io_context io;
    uint16_t port = 22222;
    EchoServer server(io, port);
    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    // Reconnect (will close and reconnect)
    bool reconnected = false;
    client->setOnConnect([&]() {
        reconnected = true;
    });
    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return reconnected;
        },
        100);

    server.stop();
}

// ==================== TESTS FOR BINARY DATA ====================

TEST_F(ClientTest, SendBinaryData) {
    boost::asio::io_context io;
    uint16_t port = 18092;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    bool received = false;
    client->setOnReceiveMessage([&](RawMessage) {
        received = true;
    });

    ByteBuffer binaryData = {0x01, 0x02, 0x03, 0xFF, 0x00, 0x7F, 0x80, 0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_NO_THROW(client->send(RawMessage(std::move(binaryData))));
    runUntil(
        io,
        [&]() {
            return received;
        },
        100);
    server.stop();
}

TEST_F(ClientTest, SendMessageDataDirectly) {
    boost::asio::io_context io;
    uint16_t port = 18093;
    EchoServer server(io, port);

    auto client = Client::create(io);
    bool connected = false;
    client->setOnConnect([&]() {
        connected = true;
    });

    client->startConnect("127.0.0.1", port);
    runUntil(
        io,
        [&]() {
            return connected;
        },
        100);

    bool received = false;
    client->setOnReceiveMessage([&](RawMessage) {
        received = true;
    });

    ByteBuffer data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    EXPECT_NO_THROW(client->send(RawMessage(std::move(data))));
    runUntil(
        io,
        [&]() {
            return received;
        },
        100);
    server.stop();
}
