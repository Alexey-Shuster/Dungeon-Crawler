#pragma once

#include <common/network/raw_message.h>
#include <common/wire/serder.h>
#include <memory>
#include <server/core/event_base.h>

namespace dungeons::server::app {

/**
 * @brief Создание события по типу сообщения и аргументам
 *
 * @details Использует kEventFactoryMap для поиска фабрики, соответствующей типу сообщения.
 *          Если фабрика найдена, она вызывается с переданными аргументами.
 *
 * @note Возвращает nullptr в следующих случаях:
 *       - msg_type == std::monostate (неизвестный тип)
 *       - Нет зарегистрированной фабрики для msg_type
 *       - Фабрика вернула nullptr (невалидные аргументы)
 */
[[nodiscard]] std::shared_ptr<core::Event> makeEvent(const dc_MsgVariant& msg_type, common::wire::MessageArgs msg_args);

/// @brief Десериализация бинарного сообщения в событие
[[nodiscard]] std::shared_ptr<core::Event> deserializeMessage(common::network::RawMessage message);

}  // namespace dungeons::server::app
