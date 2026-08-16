#pragma once

#include <cbor.h>
#include <common/network/byte_buffer.h>
#include <memory>
#include <optional>
#include <vector>

namespace dungeons::common::wire {

/**
 * @brief Тип для хранения аргументов сообщения
 *
 * std::optional используется для различения:
 * - std::nullopt: ошибка при извлечении аргументов
 * - пустой вектор: аргументы отсутствуют (только тип сообщения)
 * - вектор с данными: аргументы присутствуют
 */
using MessageArgs = std::optional<std::vector<uint64_t>>;

}  // namespace dungeons::common::wire

namespace dungeons::common::wire::detail {

/**
 * @brief Делегатор для освобождения CBOR-объектов.
 *
 * Используется с std::unique_ptr для автоматического управления
 * временем жизни CBOR-объектов через cbor_decref.
 */
struct CborDeleter {
    /**
     * @brief Оператор вызова для удаления CBOR-объекта
     * @param item Указатель на CBOR-объект (может быть nullptr)
     */
    void operator()(cbor_item_t* item) const {
        if (item) {
            cbor_decref(&item);
        }
    }
};

/**
 * @brief Умный указатель для автоматического управления CBOR-объектами.
 *
 * Использует CborDeleter для корректного освобождения памяти.
 */
using CborPtr = std::unique_ptr<cbor_item_t, CborDeleter>;

/**
 * @brief Безопасное чтение целого числа из CBOR-элемента.
 *
 * @note Использует cbor_int_get_width для определения разрядности и вызывает
 *       соответствующий геттер, избегая assert-ов libcbor.
 */
std::optional<uint64_t> readCborUint(cbor_item_t* item);

/**
 * @brief Function templates for safe and convenient creation of
 * CBOR arrays using Fold Expressions
 */
template <typename... Args>
bool pushItems(cbor_item_t* array, Args&&... args) {
    return (cbor_array_push(array, args) && ...);
}

template <typename... Args>
CborPtr buildArray(Args&&... args) {
    auto arr = CborPtr(cbor_new_definite_array(sizeof...(args)));
    if (!arr)
        return nullptr;
    if (!pushItems(arr.get(), args...))
        return nullptr;
    return arr;
}

/**
 * @brief Serializes a CBOR item to a ByteBuffer (binary).
 *
 * @details Calls cbor_serialize_alloc and wraps the result into a ByteBuffer.
 *          Logs errors internally.
 */
std::optional<network::ByteBuffer> cborToMessage(CborPtr root);

/**
 * @brief Loads a ByteBuffer into a CBOR item.
 *
 * @details Calls cbor_load and checks for errors. The load result can be inspected
 *          via out_result if provided.
 */
CborPtr bufferToCbor(network::ByteBuffer buffer, cbor_load_result* out_result = nullptr);

/**
 * @brief Get a value from a CBOR map by string key.
 *
 * @warning Uses cbor_map_handle, which is not officially part of the public API.
 *          Better use libcbor 0.14 cbor_map_get
 */
CborPtr cborMapGet(cbor_item_t* map, const char* key);

}  // namespace dungeons::common::wire::detail
