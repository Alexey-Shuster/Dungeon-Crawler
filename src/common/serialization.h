#pragma once
/**
 * @file serialization.h
 * @brief Сериализация и десериализация сообщений в формате CBOR
 *
 * Этот модуль предоставляет функции для преобразования сообщений между
 * внутренним представлением (события) и бинарным форматом CBOR.
 *
 * @author DRUsmanov
 * @author Alexey-Shuster
 * @version 1.1 (адаптировано под message::MessageTypeVariant)
 */

#include <cbor.h>
#include <format>
#include <memory>
#include <optional>
#include <vector>

#include "logger.h"
#include "message.h"
#include "message_types.h"
#include "message_utils.h"
#include "serialization_utils.h"

namespace serialization {

/**
 * @brief Добавление 64-битного беззнакового аргумента в CBOR-массив
 *
 * @param array Указатель на CBOR-массив
 * @param value 64-битное беззнаковое значение для добавления
 * @return true Успешное добавление
 * @return false Ошибка создания CBOR-элемента или добавления в массив
 *
 * @note В случае ошибки логирует проблему
 * @warning Не проверяет размер массива (предполагается, что он создан с достаточным количеством элементов)
 */
inline bool addArgToArray(cbor_item_t* array, uint64_t value) {
    // TODO попробовать сделать через append(cbor, value) для разных типов через перегрузки - комментарий и задача
    CborPtr item = CborPtr(cbor_build_uint64(value));
    if (!item) {
        LOG_ERROR("Failed to create CBOR item for argument");
        return false;
    }
    if (!cbor_array_push(array, item.get())) {
        LOG_ERROR("Failed to add argument to CBOR array");
        return false;
    }
    return true;
}

/**
 * @brief Извлечение аргументов сообщения из CBOR-массива
 *
 * @param array Указатель на CBOR-массив (ожидается, что первый элемент - тип сообщения)
 * @return MessageArgs Аргументы сообщения в виде вектора uint64_t
 *
 * @details Функция извлекает все элементы массива, начиная с индекса 1 (пропуская тип сообщения).
 *          Каждый аргумент должен быть беззнаковым целым числом (uint64_t).
 *
 * @note Возвращает std::nullopt при ошибке
 *
 * @retval std::nullopt Ошибка: массив пуст, не является массивом, или содержит не uint64_t элементы
 * @retval std::vector<uint64_t> (пустой вектор) Массив содержит только тип сообщения (нет аргументов)
 * @retval std::vector<uint64_t> Вектор с аргументами
 */
inline MessageArgs getMsgArgsFromCborArray(cbor_item_t* array) {
    if (!array || !cbor_isa_array(array)) {
        LOG_ERROR("Cbor data is not array");
        return std::nullopt;
    }
    size_t array_size = cbor_array_size(array);

    if (array_size == 0) {
        LOG_ERROR("Cbor array is empty");
        return std::nullopt;
    }

    if (array_size == 1) {
        return std::vector<uint64_t>{};
    }

    std::vector<uint64_t> msg_args;
    msg_args.reserve(array_size - 1);
    for (size_t i = 1; i < array_size; ++i) {
        auto msg_arg_item = CborPtr(cbor_array_get(array, i));
        if (msg_arg_item && cbor_isa_uint(msg_arg_item.get())) {
            auto value_opt = readCborUint(msg_arg_item.get());
            if (!value_opt) {
                LOG_ERROR(std::format("Failed to read message arg {}", i));
                return std::nullopt;
            }
            msg_args.push_back(*value_opt);
        } else {
            LOG_ERROR(std::format("Message arg {} in cbor is not uint64_t", i));
            return std::nullopt;
        }
    }
    return msg_args;
}

/**
 * @brief Creates a CBOR definite array with the packed message type as its first element.
 * @param msg_type The message type variant (must not be monostate).
 * @param numArgs The number of argument elements to reserve after the type.
 * @return CborPtr A valid CBOR array with the type already pushed, or nullptr on error.
 *
 * @details This function:
 *          - Checks that msg_type is not std::monostate.
 *          - Packs the type via message::packMessageType.
 *          - Creates a definite array of size 1 + numArgs.
 *          - Pushes the packed type as the first element.
 *
 * @note Logs all errors internally; caller only needs to check for nullptr.
 */
inline CborPtr createMessageArray(const message::MessageTypeVariant& msg_type, size_t numArgs) {
    if (std::holds_alternative<std::monostate>(msg_type)) {
        LOG_ERROR("Cannot serialize message with unknown type");
        return nullptr;
    }

    auto packed = message::packMessageType(msg_type);
    if (!packed) {
        LOG_ERROR("Failed to pack message type");
        return nullptr;
    }

    auto msg_array = CborPtr(cbor_new_definite_array(1 + numArgs));
    if (!msg_array) {
        LOG_ERROR("Failed to create CBOR array");
        return nullptr;
    }

    auto msg_type_item = CborPtr(cbor_build_uint64(*packed));
    if (!msg_type_item) {
        LOG_ERROR("Failed to create CBOR item for message type");
        return nullptr;
    }

    if (!cbor_array_push(msg_array.get(), msg_type_item.get())) {
        LOG_ERROR("Failed to add message type to CBOR array");
        return nullptr;
    }

    return msg_array;
}

/**
 * @brief Serializes a message with a compile-time known number of arguments.
 *
 * @tparam Args Types of arguments (must be convertible to uint64_t)
 * @param msg_type The message type (must not be std::monostate)
 * @param args The argument values
 * @return std::optional<network::Message> Serialized message, or nullopt on error
 *
 * @details Creates a CBOR array: [type, arg0, arg1, ...].
 *
 * @note On error, logs internally and returns nullopt.
 */
template <typename... Args>
std::optional<network::Message> serializeMessage(const message::MessageTypeVariant& msg_type, Args... args) {
    static_assert((std::is_convertible_v<Args, uint64_t> && ...), "All arguments must be convertible to uint64_t");

    auto msg_array = createMessageArray(msg_type, sizeof...(args));
    if (!msg_array) {
        LOG_ERROR("Failed to create CBOR array for provided message");
        return std::nullopt;
    };

    if (!((addArgToArray(msg_array.get(), args)) && ...)) {
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
 * @param msg_type The message type (must not be std::monostate)
 * @param args The arguments (each must fit in uint64_t)
 * @return std::optional<network::Message> Serialized message, or nullopt on error
 *
 * @details Creates a CBOR array: [type, arg0, arg1, ...].
 *
 * @see serializeMessage (variadic version)
 */
inline std::optional<network::Message> serializeMessage(const message::MessageTypeVariant& msg_type,
                                                        const std::vector<uint64_t>& args) {
    auto msg_array = createMessageArray(msg_type, args.size());
    if (!msg_array) {
        LOG_ERROR("Failed to create CBOR array for provided message");
        return std::nullopt;
    };

    for (uint64_t val : args) {
        if (!addArgToArray(msg_array.get(), val)) {
            LOG_ERROR("Failed to add argument to CBOR array");
            return std::nullopt;
        }
    }

    return cborToMessage(std::move(msg_array));
}

/**
 * @brief Десериализация сообщения из формата CBOR (сырой результат)
 *
 * @param message Сообщение в формате CBOR (содержит бинарные данные)
 * @return std::optional<DeserializedMessage> Структура с типом и аргументами,
 *         или std::nullopt при ошибке.
 *
 * @details Функция выполняет следующие шаги:
 *          1. Загрузка CBOR-данных из Message
 *          2. Проверка, что данные являются CBOR-массивом
 *          3. Проверка, что массив не пустой
 *          4. Извлечение типа сообщения (первый элемент, uint16_t)
 *          5. Извлечение аргументов (остальные элементы)
 *
 * @note Возвращает сырые данные без создания события.
 *       Для создания события использовать message_dispatcher::makeEvent.
 *
 * @see serializeMessage
 */
struct DeserializedMessage {
    message::MessageTypeVariant type;
    std::vector<uint64_t> args;
};

// TODO: validate message protocol (args number, args type?)
inline std::optional<DeserializedMessage> deserializeMessageRaw(const network::Message& message) {
    cbor_load_result load_result{};
    auto msg_array = messageToCbor(message, &load_result);

    if (!msg_array || load_result.error.code != CBOR_ERR_NONE) {
        LOG_ERROR("Failed to load CBOR data");
        return std::nullopt;
    }

    if (!cbor_isa_array(msg_array.get())) {
        LOG_ERROR("Cbor data is not array");
        return std::nullopt;
    }

    if (cbor_array_size(msg_array.get()) == 0) {
        LOG_ERROR("Cbor array is empty");
        return std::nullopt;
    }

    // Читаем тип (теперь uint16_t)
    auto msg_type_item = CborPtr(cbor_array_get(msg_array.get(), 0));
    if (!msg_type_item || !cbor_isa_uint(msg_type_item.get())) {
        LOG_ERROR("Message type in cbor is not unsigned integer");
        return std::nullopt;
    }

    auto packed_opt = readCborUint(msg_type_item.get());
    if (!packed_opt) {
        LOG_ERROR("Failed to read message type value");
        return std::nullopt;
    }
    auto packed_type = static_cast<uint16_t>(*packed_opt);

    auto msg_type = message::unpackMessageType(packed_type);
    if (!msg_type) {
        LOG_ERROR(std::format("Failed to unpack message type from {}", packed_type));
        return std::nullopt;
    }

    auto msg_args = getMsgArgsFromCborArray(msg_array.get());
    if (!msg_args) {
        LOG_ERROR("Failed to extract arguments from CBOR array");
        return std::nullopt;
    }

    return DeserializedMessage{*msg_type, *msg_args};
}

}  // namespace serialization
