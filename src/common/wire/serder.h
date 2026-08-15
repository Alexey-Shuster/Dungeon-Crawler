#pragma once
/**
 * @file serder.h
 * @brief Сериализация и десериализация сообщений в формате CBOR
 *
 * @author DRUsmanov
 * @author Alexey-Shuster
 * @version 1.1 (адаптировано под message::MessageTypeVariant & ByteBuffer)
 */

#include <cbor.h>
#include <memory>
#include <network/byte_buffer.h>
#include <optional>
#include <types/message_types.h>
#include <utility/logger.h>
#include <vector>

#include "serder_base.h"

namespace dungeons::common::wire {

namespace detail {

/**
 * @brief Добавление 64-битного беззнакового аргумента в CBOR-массив
 *
 * @note В случае ошибки логирует проблему
 * @warning Не проверяет размер массива (предполагается, что он создан с достаточным количеством элементов)
 */
bool addArgToArray(cbor_item_t* array, uint64_t value);

/**
 * @brief Извлечение аргументов сообщения из CBOR-массива
 *
 * @details Функция извлекает все элементы массива, начиная с индекса 1 (пропуская тип сообщения).
 *          Каждый аргумент должен быть беззнаковым целым числом (uint64_t).
 *
 * @retval std::nullopt Ошибка: массив пуст, не является массивом, или содержит не uint64_t элементы
 * @retval std::vector<uint64_t> (пустой вектор) Массив содержит только тип сообщения (нет аргументов)
 * @retval std::vector<uint64_t> Вектор с аргументами
 */
MessageArgs getMsgArgsFromCborArray(cbor_item_t* array);

/**
 * @brief Creates a CBOR definite array with the packed message type as its first element.
 *
 * @details This function:
 *          - Checks that msg_type is not std::monostate.
 *          - Packs the type via packMessageType.
 *          - Creates a definite array of size 1 + numArgs.
 *          - Pushes the packed type as the first element.
 *
 * @note Logs all errors internally; caller only needs to check for nullptr.
 */
CborPtr createMessageArray(const types::MessageTypeVariant& msg_type, size_t numArgs);

}  // namespace detail

/**
 * @brief Serializes a message with a compile-time known number of arguments.
 *
 * @details Creates a CBOR array: [type, arg0, arg1, ...].
 *
 * @note On error, logs internally and returns nullopt.
 */
template <typename... Args>
std::optional<network::ByteBuffer> serializeMessage(const types::MessageTypeVariant& msg_type, Args... args) {
    static_assert((std::is_convertible_v<Args, uint64_t> && ...), "All arguments must be convertible to uint64_t");

    auto msg_array = detail::createMessageArray(msg_type, sizeof...(args));
    if (!msg_array) {
        LOG_ERROR("Failed to create CBOR array for provided message");
        return std::nullopt;
    };

    if (!((detail::addArgToArray(msg_array.get(), args)) && ...)) {
        LOG_ERROR("Failed to add one or more arguments to CBOR array");
        return std::nullopt;
    }

    return cborToMessage(std::move(msg_array));
}

/**
 * @brief Serializes a message with a vector of arguments (runtime-known count).
 *
 * This overload is intended for messages where the number of arguments is dynamic
 * (e.g., a list of lobby IDs). The vector is flattened into the CBOR array as separate
 * uint64_t elements, maintaining the same wire format as the variadic version.
 *
 * @details Creates a CBOR array: [type, arg0, arg1, ...].
 *
 * @see serializeMessage (variadic version)
 */
std::optional<network::ByteBuffer> serializeMessage(const types::MessageTypeVariant& msg_type,
                                                    const std::vector<uint64_t>& args);

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
    types::MessageTypeVariant type;
    std::vector<uint64_t> args;
};
// TODO: validate message protocol (args number, args type?)
std::optional<DeserializedRawMessage> deserializeRawMessage(network::ByteBuffer buffer);

}  // namespace dungeons::common::wire
