#pragma once

#include <common/network/byte_buffer.h>
#include <common/types/message_types.h>
#include <optional>
#include <vector>

namespace dungeons::common::wire {

/// @brief Тип для хранения аргументов сообщения
using MessageArgs = std::vector<uint64_t>;

/**
 * @brief Serializes a message with a vector of arguments (runtime-known count).
 *
 * @details Creates a CBOR array: [type, arg0, arg1, ...].
 */
std::optional<network::ByteBuffer> serializeMessageToBuffer(const dc_MsgVariant& msg_type, MessageArgs&& args);

/// @brief Serializes a message with a compile-time known number of arguments
template <typename... Args>
std::optional<network::ByteBuffer> serializeMessageToBuffer(const dc_MsgVariant& msg_type, Args... args) {
    static_assert((std::is_convertible_v<Args, uint64_t> && ...), "All arguments must be convertible to uint64_t");
    MessageArgs vec{static_cast<uint64_t>(args)...};
    return serializeMessageToBuffer(msg_type, std::move(vec));
}

/**
 * @brief Десериализация сообщения из формата CBOR (сырой результат)
 *
 * @details Функция выполняет следующие шаги:
 *          1. Загрузка CBOR-данных из Message
 *          2. Проверка, что данные являются CBOR-массивом
 *          3. Проверка, что массив не пустой
 *          4. Извлечение типа сообщения (первый элемент, uint16_t)
 *          5. Извлечение аргументов (остальные элементы)
 *
 * @see serializeMessage
 */
struct DeserializedRawMessage {
    dc_MsgVariant type;
    MessageArgs args;
};
// TODO: validate message protocol (args number, args type?)
std::optional<DeserializedRawMessage> deserializeBufferToMessage(network::ByteBuffer buffer);

}  // namespace dungeons::common::wire
