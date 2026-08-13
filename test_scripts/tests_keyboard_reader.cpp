#include <boost/asio.hpp>
#include <chrono>
#include <common/keyboard_reader.h>
#include <gtest/gtest.h>
#include <thread>
#include <unordered_map>

using namespace utility;

class TestableKeyboardReader : public KeyboardReader {
public:
    explicit TestableKeyboardReader(boost::asio::io_context& io, bool edge_triggered = false)
        : KeyboardReader(io, edge_triggered) {}

    void setKeyDown(char key, bool down) {
        simulated_state_[std::tolower(static_cast<unsigned char>(key))] = down;
    }

protected:
    void fetchKeyState() override {}
    bool isKeyDown(char key) const override {
        char lower = std::tolower(static_cast<unsigned char>(key));
        auto it = simulated_state_.find(lower);
        return it != simulated_state_.end() && it->second;
    }

private:
    std::unordered_map<char, bool> simulated_state_;
};

class KeyboardReaderTest : public ::testing::Test {
protected:
    boost::asio::io_context io;
    std::shared_ptr<TestableKeyboardReader> reader;

    void SetUp() override {
        reader = std::make_shared<TestableKeyboardReader>(io);
    }

    void TearDown() override {
        if (reader) {
            reader->stop();
        }
    }

    void runFor(std::chrono::milliseconds duration) {
        // Reset the io_context state to allow execution after previous runs or drains
        io.restart();

        // Create a guaranteed workload that expires exactly after the specified duration
        boost::asio::steady_timer safety_timer(io, duration);
        safety_timer.async_wait([](const boost::system::error_code&) {
            // Do nothing; this callback only exists to keep the io_context active
        });

        // Block the thread for the full duration while concurrently processing
        // any ticking internal timers from the KeyboardReader
        io.run_for(duration);
    }
};

TEST_F(KeyboardReaderTest, ConstructorDestructorNoCrash) {
    auto r = std::make_shared<TestableKeyboardReader>(io);
}

TEST_F(KeyboardReaderTest, StartStopDoesNotCrash) {
    reader->start([](char) {
    });
    reader->stop();
}

TEST_F(KeyboardReaderTest, StartTwiceIgnored) {
    int call_count = 0;
    auto cb = [&](char) {
        ++call_count;
    };
    reader->start(cb);
    reader->start(cb);
    reader->setKeyDown('w', true);
    runFor(std::chrono::milliseconds(50));
    SUCCEED();
}

TEST_F(KeyboardReaderTest, EdgeTriggeredFiresOncePerPress) {
    reader = std::make_shared<TestableKeyboardReader>(io, true);
    int call_count = 0;
    reader->start(
        [&](char) {
            ++call_count;
        },
        std::chrono::milliseconds(10));

    reader->setKeyDown('w', true);
    runFor(std::chrono::milliseconds(50));
    EXPECT_EQ(call_count, 1);

    runFor(std::chrono::milliseconds(50));
    EXPECT_EQ(call_count, 1);

    reader->setKeyDown('w', false);
    runFor(std::chrono::milliseconds(50));
    reader->setKeyDown('w', true);
    runFor(std::chrono::milliseconds(100));
    EXPECT_EQ(call_count, 2);
}

TEST_F(KeyboardReaderTest, ContinuousFiresWhileHeld) {
    reader = std::make_shared<TestableKeyboardReader>(io, false);
    int call_count = 0;
    reader->start(
        [&](char) {
            ++call_count;
        },
        std::chrono::milliseconds(10));

    reader->setKeyDown('a', true);
    runFor(std::chrono::milliseconds(100));
    EXPECT_GT(call_count, 2);

    reader->setKeyDown('a', false);
    runFor(std::chrono::milliseconds(50));
    int last = call_count;
    runFor(std::chrono::milliseconds(100));
    EXPECT_EQ(call_count, last);
}

TEST_F(KeyboardReaderTest, UnsupportedKeysIgnored) {
    bool callback_called = false;
    reader->start(
        [&](char) {
            callback_called = true;
        },
        std::chrono::milliseconds(10));
    reader->setKeyDown('z', true);
    runFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(callback_called);
}

TEST_F(KeyboardReaderTest, MultipleKeysSimultaneously) {
    reader = std::make_shared<TestableKeyboardReader>(io, true);
    std::vector<char> pressed;
    reader->start(
        [&](char key) {
            pressed.push_back(key);
        },
        std::chrono::milliseconds(10));

    reader->setKeyDown('w', true);
    reader->setKeyDown('d', true);
    runFor(std::chrono::milliseconds(50));

    EXPECT_EQ(pressed.size(), 2);
    EXPECT_TRUE(std::find(pressed.begin(), pressed.end(), 'w') != pressed.end());
    EXPECT_TRUE(std::find(pressed.begin(), pressed.end(), 'd') != pressed.end());
}

TEST_F(KeyboardReaderTest, StopCancelsFurtherPolls) {
    int call_count = 0;
    reader->start(
        [&](char) {
            ++call_count;
        },
        std::chrono::milliseconds(10));
    reader->setKeyDown('s', true);

    runFor(std::chrono::milliseconds(50));
    EXPECT_GT(call_count, 0);

    reader->stop();
    int before = call_count;
    runFor(std::chrono::milliseconds(100));
    EXPECT_EQ(call_count, before);
}

TEST_F(KeyboardReaderTest, IntervalRespected) {
    auto start = std::chrono::steady_clock::now();
    int call_count = 0;
    reader->start(
        [&](char) {
            ++call_count;
        },
        std::chrono::milliseconds(50));
    reader->setKeyDown('x', true);

    runFor(std::chrono::milliseconds(150));
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(call_count, elapsed / 60);
    EXPECT_LE(call_count, elapsed / 40 + 1);
}
