/**
 * @file message_dispatcher.h
 * @brief Преобразование между сетевыми сообщениями и событиями приложения.
 *
 * Содержит фабрику событий и функцию makeEvent, которая создаёт событие
 * по типу сообщения и аргументам. Использует serialization.h для низкоуровневой
 * работы с CBOR.
 *
 * @author Alexey-Shuster
 * @version 1.1 (адаптировано под message::MessageTypeVariant)
 */

#pragma once

#include <common/network/raw_message.h>
#include <common/types/direction.h>
#include <common/types/message_utils.h>
#include <common/utility/logger.h>
#include <common/wire/serder.h>
#include <functional>
#include <memory>
#include <optional>
#include <server/core/event_base.h>
#include <server/domain/core/events.h>
#include <server/domain/core/types.h>
#include <type_traits>
#include <unordered_map>

#include "events.h"

namespace dungeons::server::app {

struct MessageTypeVariantHash {
    size_t operator()(const common::types::MessageTypeVariant& v) const {
        auto packed = packMessageType(v);
        return packed ? static_cast<size_t>(*packed) : 0;
    }
};

// ----------------------------------------------------------------------------
// Фабрика событий
// ----------------------------------------------------------------------------
using EventFactory = std::function<std::shared_ptr<core::Event>(const common::wire::MessageArgs&)>;

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
std::shared_ptr<core::Event> CreateEvent(const common::wire::MessageArgs& args) {
    constexpr size_t ExpectedSize = sizeof...(ArgTypes);

    if (!args.has_value() || args->size() != ExpectedSize) {
        return nullptr;
    }

    return [&]<size_t... Is>(std::index_sequence<Is...>) -> std::shared_ptr<core::Event> {
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
using MsgVariant = common::types::MessageTypeVariant;
using NetMsg = common::types::NetworkMessageType;
using AppMsg = common::types::AppMessageType;
using DmnMsg = common::types::DomainMessageType;

const std::unordered_map<MsgVariant, EventFactory, MessageTypeVariantHash> kEventFactoryMap = {
    {NetMsg::kJoin, CreateEvent<AuthRequestedEvent, network::SessionId, domain::PlayerId>},

    {NetMsg::kReconnect, CreateEvent<ReconnectRequestedEvent, network::SessionId, domain::PlayerId>},

    {AppMsg::kCreateParty, CreateEvent<domain::CreateLobbyRequestEvent, domain::PlayerId>},

    {AppMsg::kListParties, CreateEvent<domain::ListLobbiesRequestEvent, domain::PlayerId>},

    {AppMsg::kJoinParty, CreateEvent<domain::JoinLobbyRequestEvent, domain::PlayerId, domain::LobbyId>},

    {AppMsg::kLeaveParty, CreateEvent<domain::LeaveLobbyRequestEvent, domain::PlayerId>},

    {AppMsg::kStartGame, CreateEvent<domain::StartGameRequestEvent, domain::PlayerId>},

    {DmnMsg::kMove, CreateEvent<domain::MoveRequestEvent, domain::PlayerId, common::types::Direction>},

    {DmnMsg::kAttack, CreateEvent<domain::AtackRequestEvent, domain::PlayerId>},
};

/**
 * @brief Создание события по типу сообщения и аргументам
 *
 * @param msg_type Тип сообщения (MessageTypeVariant)
 * @param msg_args Аргументы сообщения (optional)
 * @return std::shared_ptr<core::Event> Указатель на созданное событие или nullptr при ошибке
 *
 * @details Использует kEventFactoryMap для поиска фабрики, соответствующей типу сообщения.
 *          Если фабрика найдена, она вызывается с переданными аргументами.
 *
 * @note Возвращает nullptr в следующих случаях:
 *       - msg_type == std::monostate (неизвестный тип)
 *       - Нет зарегистрированной фабрики для msg_type
 *       - Фабрика вернула nullptr (невалидные аргументы)
 */
[[nodiscard]] inline std::shared_ptr<core::Event> makeEvent(const MsgVariant& msg_type,
                                                            const common::wire::MessageArgs& msg_args) {
    if (std::holds_alternative<std::monostate>(msg_type)) {
        LOG_ERROR("Cannot create event from unknown message type");
        return nullptr;
    }

    auto it = kEventFactoryMap.find(msg_type);
    if (it == kEventFactoryMap.end()) {
        auto info = common::types::getLevelId(msg_type);
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
[[nodiscard]] inline std::shared_ptr<core::Event> deserializeMessage(common::network::RawMessage message) {
    auto raw = common::wire::deserializeRawMessage(std::move(message.buffer));
    if (!raw) {
        LOG_ERROR("Failed to deserialize raw message");
        return nullptr;
    }
    return makeEvent(raw->type, raw->args);
}

}  // namespace dungeons::server::app
