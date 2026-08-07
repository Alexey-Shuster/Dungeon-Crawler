/**
 * @file message_dispatcher.h
 * @brief Преобразование между сетевыми сообщениями и событиями приложения.
 *
 * Содержит фабрику событий и функцию makeEvent, которая создаёт событие
 * по типу сообщения и аргументам. Использует serialization.h для низкоуровневой
 * работы с CBOR.
 *
 * @author DRUsmanov
 * @author Alexey-Shuster
 * @version 1.1 (адаптировано под message::MessageTypeVariant)
 */

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>

#include "events.h"
#include "logger.h"
#include "message_utils.h"
#include "serialization.h"

namespace message_dispatcher {

struct MessageTypeVariantHash {
    size_t operator()(const message::MessageTypeVariant& v) const {
        auto packed = message::packMessageType(v);
        return packed ? static_cast<size_t>(*packed) : 0;
    }
};

// ----------------------------------------------------------------------------
// Фабрика событий
// ----------------------------------------------------------------------------
using EventFactory = std::function<std::shared_ptr<events::Event>(const serialization::MessageArgs&)>;

///@brief Вспомогательный хелпер
template <typename Target, typename Source>
constexpr std::optional<Target> safe_argument_cast(Source value) {
    // Обработка Enum Class
    if constexpr (std::is_enum_v<Target>) {
        using Underlying = std::underlying_type_t<Target>;

        // Проверяем текущее значение uint64_t на диапазон базового типа энума
        if (!std::in_range<Underlying>(value)) {
            return std::nullopt;
        }
        auto underlying_value = static_cast<Underlying>(value);

        return static_cast<Target>(underlying_value);
    }
    // Обработка стандартных числовых типов (int, uint32_t и т.д.)
    else {
        return static_cast<Target>(value);
    }
}

///@brief Универсальный шаблон для создания событий с проверкой аргументов
template <typename EventType, typename... ArgTypes>
std::shared_ptr<events::Event> CreateEvent(const serialization::MessageArgs& args) {
    constexpr size_t ExpectedSize = sizeof...(ArgTypes);

    if (!args.has_value() || args->size() != ExpectedSize) {
        return nullptr;
    }

    return [&]<size_t... Is>(std::index_sequence<Is...>) -> std::shared_ptr<events::Event> {
        // Получаем пакет из std::optional<ArgTypes>...
        auto casted_args = std::make_tuple(safe_argument_cast<ArgTypes>((*args)[Is])...);

        // Проверяем, что ВСЕ optional внутри кортежа содержат значения.
        // Используем свертку && для std::get<Is>(casted_args).has_value()
        bool all_valid = (std::get<Is>(casted_args).has_value() && ...);

        if (!all_valid) {
            return nullptr;
        }

        // Если всё валидно, конструируем событие, доставая значения через оператор *
        return std::make_shared<EventType>((*std::get<Is>(casted_args))...);
    }(std::make_index_sequence<ExpectedSize>{});
}

/**
 * @brief Карта фабрик событий по типу сообщения.
 *
 * Каждая фабрика проверяет количество и корректность аргументов.
 * Если аргументы невалидны, возвращает nullptr.
 *
 * @todo fix into std::array
 */
const std::unordered_map<message::MessageTypeVariant, EventFactory, MessageTypeVariantHash> kEventFactoryMap = {
    {message::NetworkMessageType::kJoin, CreateEvent<events::AuthRequestedEvent, SessionId, PlayerId>},

    {message::NetworkMessageType::kReconnect, CreateEvent<events::ReconnectRequestedEvent, SessionId, PlayerId>},

    {message::AppMessageType::kCreateParty, CreateEvent<events::CreateLobbyRequestEvent, PlayerId>},

    {message::AppMessageType::kListParties, CreateEvent<events::ListLobbiesRequestEvent, PlayerId>},

    {message::AppMessageType::kJoinParty, CreateEvent<events::JoinLobbyRequestEvent, PlayerId, LobbyId>},

    {message::AppMessageType::kLeaveParty, CreateEvent<events::LeaveLobbyRequestEvent, PlayerId>},

    {message::AppMessageType::kStartGame, CreateEvent<events::StartGameRequestEvent, PlayerId>},

    {message::DomainMessageType::kMove, CreateEvent<events::MoveRequestEvent, PlayerId, Direction>},

    {message::DomainMessageType::kAttack, CreateEvent<events::AtackRequestEvent, PlayerId>},
};

/**
 * @brief Создание события по типу сообщения и аргументам
 *
 * @param msg_type Тип сообщения (MessageTypeVariant)
 * @param msg_args Аргументы сообщения (optional)
 * @return std::shared_ptr<events::Event> Указатель на созданное событие или nullptr при ошибке
 *
 * @details Использует kEventFactoryMap для поиска фабрики, соответствующей типу сообщения.
 *          Если фабрика найдена, она вызывается с переданными аргументами.
 *
 * @note Возвращает nullptr в следующих случаях:
 *       - msg_type == std::monostate (неизвестный тип)
 *       - Нет зарегистрированной фабрики для msg_type
 *       - Фабрика вернула nullptr (невалидные аргументы)
 */
[[nodiscard]] inline std::shared_ptr<events::Event> makeEvent(const message::MessageTypeVariant& msg_type,
                                                              const serialization::MessageArgs& msg_args) {
    if (std::holds_alternative<std::monostate>(msg_type)) {
        LOG_ERROR("Cannot create event from unknown message type");
        return nullptr;
    }

    auto it = kEventFactoryMap.find(msg_type);
    if (it == kEventFactoryMap.end()) {
        auto info = message::getLevelId(msg_type);
        if (info) {
            LOG_ERROR(std::format("No event factory for type (level={}, id={})",
                                  static_cast<uint8_t>(info->level),
                                  info->id));
        } else {
            LOG_ERROR("No event factory for invalid message type");
        }
        return nullptr;
    }
    return it->second(msg_args);
}

// ----------------------------------------------------------------------------
// Десериализация (сообщение → событие)
// ----------------------------------------------------------------------------

/**
 * @brief Десериализация сообщения из формата CBOR в событие
 *
 * @param message Сообщение в формате CBOR (содержит бинарные данные)
 * @return std::shared_ptr<events::Event> Указатель на созданное событие или nullptr при ошибке
 *
 * @details Объединяет deserializeMessageRaw из serialization.h и makeEvent.
 *
 * @see serialization::deserializeMessageRaw
 * @see makeEvent
 */
[[nodiscard]] inline std::shared_ptr<events::Event> deserializeMessage(const network::Message& message) {
    auto raw = serialization::deserializeMessageRaw(message);
    if (!raw) {
        LOG_ERROR("Failed to deserialize raw message");
        return nullptr;
    }
    return makeEvent(raw->type, raw->args);
}

}  // namespace message_dispatcher
