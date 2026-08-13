#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <common/logger.h>
#include <format>
#include <memory>

#include "event_bus.h"
#include "events.h"

namespace dungeons::server::core {

class GameLoop : public std::enable_shared_from_this<GameLoop> {
public:
    GameLoop(boost::asio::io_context& ioc, std::shared_ptr<core::EventBus> bus, std::chrono::milliseconds rate)
        : io_context_(ioc)
        , event_bus_(std::move(bus))
        , tick_rate_(rate)
        , timer_(io_context_) {
        if (!event_bus_) {
            throw std::invalid_argument("EventBus cannot be null");
        }
    }

    ~GameLoop() {
        stop();
    }

    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;
    GameLoop(GameLoop&&) = delete;
    GameLoop& operator=(GameLoop&&) = delete;

    void start() {
        stopped_ = false;
        LOG_INFO(std::format("GameLoop started at {}", std::chrono::system_clock::now()));
        scheduleTick();
    }

    void stop() {
        stopped_ = true;
        LOG_INFO(std::format("GameLoop stopped at {}", std::chrono::system_clock::now()));
        timer_.cancel();
    }

private:
    boost::asio::io_context& io_context_;
    std::shared_ptr<core::EventBus> event_bus_;
    std::chrono::milliseconds tick_rate_;
    boost::asio::steady_timer timer_;
    std::atomic<bool> stopped_{false};

    void scheduleTick() {
        if (stopped_)
            return;

        timer_.expires_after(tick_rate_);
        timer_.async_wait([self = shared_from_this(), this](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted || stopped_) {
                // Timer was cancelled (stop called)
                return;
            }
            if (ec) {
                LOG_ERROR(
                    std::format("GameLoop timer error at {} : {}", ec.message(), std::chrono::system_clock::now()));
                // Try to reschedule anyway
                scheduleTick();
                return;
            }

            const auto now = std::chrono::steady_clock::now();
            GameTickEvent event(now);
            LOG_INFO(std::format("Publishing GameTickEvent at {}", std::chrono::system_clock::now()));
            event_bus_->publish(event);

            // Schedule next tick (unless stopped)
            if (!stopped_) {
                scheduleTick();
            }
        });
    }
};

}  // namespace dungeons::server::core
