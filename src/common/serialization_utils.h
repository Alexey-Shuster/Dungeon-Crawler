#pragma once

#include <cbor.h>
#include <memory>
#include <optional>
#include <vector>

#include "byte_buffer.h"
#include "logger.h"

namespace network {

/**
 * @brief Делегатор для освобождения CBOR-объектов
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
 * @brief Умный указатель для автоматического управления CBOR-объектами
 *
 * Использует CborDeleter для корректного освобождения памяти.
 */
using CborPtr = std::unique_ptr<cbor_item_t, CborDeleter>;

/**
 * @brief Тип для хранения аргументов сообщения
 *
 * std::optional используется для различения:
 * - std::nullopt: ошибка при извлечении аргументов
 * - пустой вектор: аргументы отсутствуют (только тип сообщения)
 * - вектор с данными: аргументы присутствуют
 */
using MessageArgs = std::optional<std::vector<uint64_t>>;

/**
 * @brief Безопасное чтение целого числа из CBOR-элемента
 * @param item Указатель на CBOR-элемент
 * @return std::optional<uint64_t> Значение или std::nullopt, если элемент не является целым числом
 *
 * @note Использует cbor_int_get_width для определения разрядности и вызывает
 *       соответствующий геттер, избегая assert-ов libcbor.
 */
inline std::optional<uint64_t> readCborUint(cbor_item_t* item) {
    if (!item || !cbor_isa_uint(item))
        return std::nullopt;
    switch (cbor_int_get_width(item)) {
        case CBOR_INT_8:
            return cbor_get_uint8(item);
        case CBOR_INT_16:
            return cbor_get_uint16(item);
        case CBOR_INT_32:
            return cbor_get_uint32(item);
        case CBOR_INT_64:
            return cbor_get_uint64(item);
        default:
            return std::nullopt;
    }
}

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
 * @param root The CBOR root item (ownership is moved into the function).
 * @return std::optional<ByteBuffer> The binary byte-buffer, or std::nullopt on error.
 *
 * @details Calls cbor_serialize_alloc and wraps the result into a ByteBuffer.
 *          Logs errors internally.
 */
inline std::optional<ByteBuffer> cborToMessage(CborPtr root) {
    if (!root) {
        LOG_ERROR("Cannot serialize null CBOR item");
        return std::nullopt;
    }
    uint8_t* buffer = nullptr;
    size_t buffer_size = 0;
    if (cbor_serialize_alloc(root.get(), &buffer, &buffer_size) == 0) {
        free(buffer);
        LOG_ERROR("Failed to serialize CBOR data");
        return std::nullopt;
    }
    std::unique_ptr<uint8_t, decltype(&free)> buffer_guard(buffer, &free);
    std::vector<uint8_t> data(buffer_guard.get(), buffer_guard.get() + buffer_size);

    return ByteBuffer{std::move(data)};
}

/**
 * @brief Loads a ByteBuffer into a CBOR item.
 * @param buffer The binary byte-buffer.
 * @param out_result Optional pointer to store the load result (error code, etc.).
 * @return CborPtr The loaded CBOR item, or nullptr on failure.
 *
 * @details Calls cbor_load and checks for errors. The load result can be inspected
 *          via out_result if provided.
 */
inline CborPtr bufferToCbor(const ByteBuffer& buffer, cbor_load_result* out_result = nullptr) {
    cbor_load_result result;
    CborPtr root(cbor_load(buffer.data(), buffer.size(), &result));
    if (out_result) {
        *out_result = result;
    }
    if (!root || result.error.code != CBOR_ERR_NONE || result.read != buffer.size()) {
        return nullptr;
        // result.error.code can be checked by caller if needed.
    }
    return root;
}

/**
 * @brief Get a value from a CBOR map by string key.
 * @param map The CBOR map (must be a map).
 * @param key The key string (null‑terminated).
 * @return CborPtr The value item (with incref) or nullptr if not found or error.
 *
 * @warning Uses cbor_map_handle, which is not officially part of the public API.
 *          Better use libcbor 0.14 cbor_map_get
 */
inline CborPtr cborMapGet(cbor_item_t* map, const char* key) {
    if (!map || !cbor_isa_map(map)) {
        return nullptr;
    }

    size_t map_size = cbor_map_size(map);
    struct cbor_pair* pairs = cbor_map_handle(map);
    if (!pairs) {
        return nullptr;
    }

    size_t key_len = std::strlen(key);

    for (size_t i = 0; i < map_size; ++i) {
        cbor_item_t* current_key = pairs[i].key;

        if (current_key && cbor_isa_string(current_key) && cbor_string_is_definite(current_key)) {
            size_t cbor_str_len = cbor_string_length(current_key);
            if (cbor_str_len == key_len) {
                unsigned char* cbor_str_data = cbor_string_handle(current_key);
                if (std::memcmp(cbor_str_data, key, key_len) == 0) {
                    cbor_item_t* value = pairs[i].value;
                    if (value) {
                        cbor_incref(value);
                        return CborPtr(value);
                    }
                }
            }
        }
    }
    return nullptr;
}

}  // namespace serialization
