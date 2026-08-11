#pragma once

#include <queue>
#include <functional>
#include <mutex>

namespace dungeon {

class CommandQueue {
public:
    using Command = std::function<void()>;

    void push(Command cmd) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(cmd));
    }

    // Returns all pending commands and clears the queue
    std::queue<Command> popAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<Command> local;
        local.swap(queue_);
        return local;
    }

private:
    std::queue<Command> queue_;
    std::mutex mutex_;
};

} // namespace dungeon
