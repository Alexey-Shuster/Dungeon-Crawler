#pragma once

#include <optional>
#include <vector>

#include "serialization_utils.h"

namespace network {

// Define a concept to check if a type can be serialized/deserialized
template <typename T>
concept Serializable = requires(const T& item, cbor_item_t* cbor) {
    { serialize(item) } -> std::same_as<CborPtr>;
    { deserialize(cbor, static_cast<T*>(nullptr)) } -> std::same_as<std::optional<T>>;
};

// concept checks for a low-level function with a type tag
template <typename T>
concept HasTagDeserializer = requires(cbor_item_t* item) {
    { deserialize(item, static_cast<T*>(nullptr)) } -> std::same_as<std::optional<T>>;
};

// redirects the call to a low-level overload by auto-adding the tag
template <typename T>
    requires HasTagDeserializer<T>
std::optional<T> deserialize(cbor_item_t* item) {
    return deserialize(item, static_cast<T*>(nullptr));
}

// Helper to serialize a vector of items
template <typename T>
    requires Serializable<T>
CborPtr serializeVector(const std::vector<T>& vec) {
    auto array = CborPtr(cbor_new_definite_array(vec.size()));
    if (!array)
        return nullptr;
    for (const auto& item : vec) {
        auto item_cbor = serialize(item);
        if (!item_cbor || !cbor_array_push(array.get(), item_cbor.get())) {
            return nullptr;
        }
    }
    return array;
}

// Helper to deserialize a vector (expects a CBOR array)
template <typename T>
    requires Serializable<T>
std::optional<std::vector<T>> deserializeVector(cbor_item_t* array) {
    if (!array || !cbor_isa_array(array))
        return std::nullopt;

    std::vector<T> result;
    result.reserve(cbor_array_size(array));

    for (size_t i = 0; i < cbor_array_size(array); ++i) {
        auto item = CborPtr(cbor_array_get(array, i));
        if (!item)
            return std::nullopt;
        // Pass a null pointer cast to T* so the compiler knows which overload to call
        auto opt = deserialize<T>(item.get());
        if (!opt)
            return std::nullopt;
        result.push_back(std::move(*opt));
    }
    return result;
}

}  // namespace serialization
