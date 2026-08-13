#pragma once

#include <atomic>
#include <boost/signals2.hpp>
#include <format>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/logger.h"
#include "event_base.h"

namespace dungeons::server::core {

/**
 * @brief Генератор уникальных идентификаторов для типов событий.
 *
 * Каждый тип события (EventType) получает свой статический ID, который
 * вычисляется один раз при первом обращении к get<T>(). ID используются
 * для индексации внутреннего вектора сигналов в EventBus.
 */
class EventTypeIdCounter {
public:
    EventTypeIdCounter() = delete;

    /// @brief Возвращает уникальный идентификатор для типа события T.
    template <typename T>
    [[nodiscard]] static size_t get() noexcept {
        static const size_t id = nextId();
        return id;
    }

private:
    /// @brief Генерирует следующий уникальный идентификатор.
    [[nodiscard]] static size_t nextId() noexcept {
        static std::atomic<size_t> id{0};
        return id.fetch_add(1, std::memory_order_relaxed);
    }
};

/**
 * @brief Потокобезопасная шина событий на основе boost::signals2.
 *
 * Позволяет подписываться на события любого типа и публиковать их.
 * Каждый тип события хранит отдельный boost::signals2::signal.
 * Все операции с сигналом потокобезопасны -
 * https://www.boost.org/doc/libs/1_84_0/doc/html/signals2/thread-safety.html
 *
 * Подписка и публикация могут происходить из разных потоков благодаря
 * использованию std::shared_mutex. При публикации колбэки вызываются
 * без удержания внутреннего мьютекса, что предотвращает дедлоки.
 *
 * Реаллокация вектора во время publish()
 * publish() успевает скопировать стабильный адрес объекта из кучи (holder_ptr)
 * до того, как отпустит shared_lock, последующий resize() вектора в другом потоке
 * переместит сами std::unique_ptr в памяти, но не уничтожит и не переместит
 * объекты SignalHolder, на которые они указывают.
 *
 * Риск разрушения EventBus во время обработки события - использован std::enable_shared_from_this
 */
class EventBus : public std::enable_shared_from_this<EventBus> {
private:
    EventBus() = default;

    /// @brief Базовый класс для полиморфного хранения сигналов в векторе.
    struct SignalHolderBase {
        virtual ~SignalHolderBase() = default;

        virtual void dispatch(const Event& event) = 0;
    };

    /// @brief Хранилище сигнала для конкретного типа события.
    template <typename EventType>
    struct SignalHolder : public SignalHolderBase {
        boost::signals2::signal<void(const EventType&)> signal;

        void dispatch(const Event& event) override {
            const auto* expected_event = dynamic_cast<const EventType*>(&event);

            if (!expected_event) {
                return;
            }

            signal(*expected_event);
        }
    };

public:
    static std::shared_ptr<EventBus> create() {
        return std::shared_ptr<EventBus>(new EventBus());
    }

    ~EventBus() = default;

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    /**
     * @brief Подписывает колбэк на события заданного типа.
     *
     * @tparam EventType Тип события, на которое подписываемся.
     * @tparam Callback Тип вызываемого объекта (функция, лямбда и т.д.).
     * @param callback Колбэк, принимающий const EventType&.
     * @return boost::signals2::connection Объект соединения для управления подпиской.
     */
    template <typename EventType, typename Callback>
    [[nodiscard]] boost::signals2::connection subscribe(Callback&& callback) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto& holder = getOrCreateSignal<EventType>();
        auto conn = holder.signal.connect(std::forward<Callback>(callback));

        LOG_INFO(std::format("[EventBus] subscribe: type={}, id={}, connected={}",
                             typeid(EventType).name(),
                             EventTypeIdCounter::get<EventType>(),
                             conn.connected()));

        return conn;
    }

    /**
     * @brief Публикует событие, вызывая все подписанные колбэки.
     *
     * @tparam EventType Тип публикуемого события.
     * @param event Ссылка на событие (передаётся в колбэки по константной ссылке).
     *
     * @note Внутренний мьютекс удерживается только на время чтения вектора сигналов.
     *       Сам вызов сигнала происходит без блокировки, что безопасно для повторных
     *       вызовов subscribe/publish внутри колбэков.
     */
    template <typename EventType>
    void publish(const EventType& event) {
        static_assert(std::is_base_of_v<Event, EventType>,
                      "EventType must be a derived class of events::Event (or Event itself)");

        SignalHolder<EventType>* holder_ptr = nullptr;

        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            const size_t type_id = EventTypeIdCounter::get<EventType>();

            LOG_INFO(std::format("[EventBus] publish<{}>: id={}, signals_.size()={}, has signal={}",
                                 typeid(EventType).name(),
                                 type_id,
                                 signals_.size(),
                                 (type_id < signals_.size() && signals_[type_id] != nullptr)));

            if (type_id >= signals_.size() || !signals_[type_id]) {
                LOG_ERROR(std::format("[EventBus] Signal not found for type {}", typeid(EventType).name()));

                return;
            }

            holder_ptr = static_cast<SignalHolder<EventType>*>(signals_[type_id].get());
        }

        /// @brief Захватываем shared_ptr, чтобы объект не удалился во время вызова сигнала
        auto self = shared_from_this();
        LOG_INFO(std::format("Calling signal for type {}", typeid(EventType).name()));
        holder_ptr->signal(event);
    }

    // Публикует событие через базовую ссылку
    void publish(const Event& event) {
        SignalHolderBase* holder_ptr = nullptr;

        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            const auto it = event_type_ids_.find(std::type_index{typeid(event)});

            LOG_INFO(std::format("[EventBus] publish(const Event&): typeid={}, found={}",
                                 typeid(event).name(),
                                 (it != event_type_ids_.end())));

            if (it == event_type_ids_.end()) {
                LOG_ERROR(std::format("[EventBus] No type_id for event type {}", typeid(event).name()));

                return;
            }

            const size_t type_id = it->second;

            holder_ptr = signals_[type_id].get();
        }

        auto self = shared_from_this();
        LOG_INFO(std::format("[EventBus] Dispatching event of type {}", typeid(event).name()));
        holder_ptr->dispatch(event);
    }

private:
    /**
     * @brief Возвращает ссылку на SignalHolder для типа EventType, создавая при необходимости.
     *
     * @tparam EventType Тип события.
     * @return SignalHolder<EventType>& Ссылка на созданный или существующий SignalHolder.
     *
     * @warning Метод предназначен для вызова только под эксклюзивной блокировкой mutex_.
     */
    template <typename EventType>
    SignalHolder<EventType>& getOrCreateSignal() {
        const size_t type_id = EventTypeIdCounter::get<EventType>();

        if (type_id >= signals_.size()) {
            signals_.resize(type_id + 1);
        }

        if (!signals_[type_id]) {
            signals_[type_id] = std::make_unique<SignalHolder<EventType>>();
        }

        event_type_ids_.try_emplace(std::type_index(typeid(EventType)), type_id);

        return *static_cast<SignalHolder<EventType>*>(signals_[type_id].get());
    }

    std::unordered_map<std::type_index, size_t> event_type_ids_;
    std::vector<std::unique_ptr<SignalHolderBase>> signals_;  ///< Вектор сигналов, индексируемый ID типа события.
    mutable std::shared_mutex
        mutex_;  ///< Мьютекс для защиты вектора signals_ (разделяемая блокировка для чтения, эксклюзивная для записи).
};

}  // namespace dungeons::server::core
