#include <utility/logger.h>

#include "serder_dtl.h"

namespace dungeons::common::wire::detail {

std::optional<uint64_t> readCborUint(cbor_item_t* item) {
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

std::optional<network::ByteBuffer> cborToMessage(CborPtr root) {
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

    return network::ByteBuffer{std::move(data)};
}

CborPtr bufferToCbor(network::ByteBuffer buffer, cbor_load_result* out_result) {
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

CborPtr cborMapGet(cbor_item_t* map, const char* key) {
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

}  // namespace dungeons::common::wire::detail
