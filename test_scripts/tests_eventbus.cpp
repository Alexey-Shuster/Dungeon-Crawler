#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "../infra/eventbus.h"

// Простые типы событий для тестирования
struct TestEvent : events::Event {
    int value;

    TestEvent() : value(0) {}
    explicit TestEvent(int v) : value(v) {}

    events::EventType getType() const override {
        return events::EventType::Ping;  // getType not used here
    }
};

struct PolymorphicTestEvent : events::Event {
    explicit PolymorphicTestEvent(int value) : value(value) {}

    int value;

    events::EventType getType() const override {
        return events::EventType::Ping;  // getType not used here
    }
};

struct AnotherEvent : events::Event {
    events::EventType getType() const override {
        return events::EventType::Ping;  // getType not used here
    }
};

// 1. Одиночная подписка – колбэк вызывается ровно один раз
TEST(EventBusTest, PublishTriggersCallback) {
    auto bus = events::EventBus::create();
    bool called = false;
    auto conn = bus->subscribe<TestEvent>([&](const TestEvent&) {
        called = true;
    });
    bus->publish(TestEvent{42});
    EXPECT_TRUE(called);
}

// 2. Несколько подписчиков на один тип – вызываются все
TEST(EventBusTest, MultipleSubscribersAllCalled) {
    auto bus = events::EventBus::create();
    int count = 0;
    auto conn1 = bus->subscribe<TestEvent>([&](const TestEvent&) {
        ++count;
    });
    auto conn2 = bus->subscribe<TestEvent>([&](const TestEvent&) {
        ++count;
    });
    bus->publish(TestEvent{});
    EXPECT_EQ(count, 2);
}

// 3. Разные типы событий не пересекаются
TEST(EventBusTest, DifferentEventTypesAreSeparate) {
    auto bus = events::EventBus::create();
    bool testCalled = false;
    bool anotherCalled = false;
    bus->subscribe<TestEvent>([&](const TestEvent&) {
        testCalled = true;
    });
    bus->subscribe<AnotherEvent>([&](const AnotherEvent&) {
        anotherCalled = true;
    });
    bus->publish(TestEvent{});
    EXPECT_TRUE(testCalled);
    EXPECT_FALSE(anotherCalled);
}

// 4. Отписка через connection – колбэк больше не вызывается
TEST(EventBusTest, DisconnectPreventsCallback) {
    auto bus = events::EventBus::create();
    int count = 0;
    auto conn = bus->subscribe<TestEvent>([&](const TestEvent&) {
        ++count;
    });
    bus->publish(TestEvent{});
    EXPECT_EQ(count, 1);
    conn.disconnect();
    bus->publish(TestEvent{});
    EXPECT_EQ(count, 1);
}

// 5. Публикация без подписчиков не приводит к ошибкам
TEST(EventBusTest, NoSubscribersNoCrash) {
    auto bus = events::EventBus::create();
    bus->publish(TestEvent{});
    SUCCEED();
}

// 6. Безопасность реентерабельности: подписка из колбэка не вызывает дедлок
TEST(EventBusTest, SubscribeDuringPublishIsSafe) {
    auto bus = events::EventBus::create();
    bool secondCalled = false;
    boost::signals2::connection conn2;
    auto conn1 = bus->subscribe<TestEvent>([&](const TestEvent& e) {
        if (e.value == 1) {
            conn2 = bus->subscribe<TestEvent>([&](const TestEvent&) {
                secondCalled = true;
            });
        }
    });
    bus->publish(TestEvent{1});  // подписываемся в процессе
    bus->publish(TestEvent{2});  // новый подписчик получает событие
    EXPECT_TRUE(secondCalled);
}

// 7. Потокобезопасность: параллельная публикация из нескольких потоков
TEST(EventBusTest, ConcurrentPublishAndSubscribe) {
    auto bus = events::EventBus::create();
    std::atomic<int> counter{0};
    const int numPublishers = 4;
    const int eventsPerPublisher = 1000;

    // Один подписчик считает все события
    bus->subscribe<TestEvent>([&](const TestEvent&) {
        ++counter;
    });

    std::vector<std::thread> threads;
    for (int i = 0; i < numPublishers; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < eventsPerPublisher; ++j) {
                bus->publish(TestEvent{});
            }
        });
    }
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(counter.load(), numPublishers * eventsPerPublisher);
}

// 8. Потокобезопасность: Разрушение EventBus во время обработки события
TEST(EventBusTest, DestroyingBusDuringPublishIsSafeWithSharedPtr) {
    auto bus = events::EventBus::create();

    std::atomic<bool> insideCallback{false};
    std::mutex mtx;
    std::condition_variable cv;

    // Подписываемся на событие, которое задержится внутри
    bus->subscribe<TestEvent>([&](const TestEvent&) {
        insideCallback = true;
        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });

    std::thread publisher([bus] {
        bus->publish(TestEvent{});
    });

    // Ждём, пока обработчик события начнёт выполняться
    {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&] {
            return insideCallback.load();
        });
    }

    // В момент, когда publisher находится внутри holder_ptr->signal(event),
    // удаляем последний shared_ptr (кроме того, что удержан в publisher)
    // Важно: bus.reset() удалит объект, но внутри publish() есть self = shared_from_this(),
    // который удерживает объект живым до конца вызова.
    bus.reset();

    publisher.join();

    // Достигли сюда без краша – UB устранено
    SUCCEED();
}

// 9. Параллельные подписка и публикация на одном типе с проверкой доставки
TEST(EventBusTest, ConcurrentSubscribeAndPublishSameType) {
    auto bus = events::EventBus::create();

    constexpr int numThreads = 8;
    constexpr int roundsPerThread = 10;
    constexpr int publishesPerRound = 100;

    std::atomic<size_t> persistentCount{0};
    std::atomic<size_t> totalPublishes{0};

    // Постоянный подписчик, который считает каждое событие
    auto persistentConn = bus->subscribe<TestEvent>([&](const TestEvent&) {
        persistentCount.fetch_add(1, std::memory_order_relaxed);
    });

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([bus, &totalPublishes] {
            std::vector<boost::signals2::connection> connections;

            for (int r = 0; r < roundsPerThread; ++r) {
                // Подписываем новый колбэк (ничего не делает, только проверяет безопасность)
                auto conn = bus->subscribe<TestEvent>([](const TestEvent&) {
                    // пустой обработчик
                });
                connections.push_back(std::move(conn));

                // Публикуем события
                for (int p = 0; p < publishesPerRound; ++p) {
                    bus->publish(TestEvent{});
                    totalPublishes.fetch_add(1, std::memory_order_relaxed);
                }
            }

            // Отписка не обязательна, но всё же)
            for (auto& c : connections) {
                c.disconnect();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    persistentConn.disconnect();

    // Постоянный подписчик должен получить каждое опубликованное событие
    EXPECT_EQ(persistentCount.load(), totalPublishes.load());
}

// 10. Полиморфная публикация через ссылку на базовый класс
TEST(EventBusTest, PolymorphicPublishTriggersTypedCallback) {
    auto bus = events::EventBus::create();
    int receivedValue = 0;

    auto connection = bus->subscribe<PolymorphicTestEvent>([&](const PolymorphicTestEvent& event) {
        receivedValue = event.value;
    });

    std::shared_ptr<events::Event> event = std::make_shared<PolymorphicTestEvent>(42);

    bus->publish(*event);

    EXPECT_EQ(receivedValue, 42);
}
