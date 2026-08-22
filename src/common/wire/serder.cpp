#include "serder.h"

#include <common/utility/logger.h>
#include <format>

#include "message_utils.h"
#include "serder_dtl.h"

namespace dungeons::common::wire {

namespace detail {

bool addArgToArray(cbor_item_t* array, uint64_t value) {
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

std::optional<MessageArgs> getMsgArgsFromCborArray(cbor_item_t* array) {
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

    MessageArgs msg_args;
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

CborPtr createMessageArray(const types::MessageTypeVariant& msg_type, size_t numArgs) {
    if (std::holds_alternative<std::monostate>(msg_type)) {
        LOG_ERROR("Cannot serialize message with unknown type");
        return nullptr;
    }

    auto packed = packMessageType(msg_type);
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

}  // namespace detail

std::optional<network::ByteBuffer> serializeMessageToBuffer(const types::MessageTypeVariant& msg_type,
                                                            MessageArgs&& args) {
    auto msg_array = detail::createMessageArray(msg_type, args.size());
    if (!msg_array) {
        LOG_ERROR("Failed to create CBOR array for provided message");
        return std::nullopt;
    };

    for (uint64_t val : args) {
        if (!detail::addArgToArray(msg_array.get(), val)) {
            LOG_ERROR("Failed to add argument to CBOR array");
            return std::nullopt;
        }
    }

    return cborToMessage(std::move(msg_array));
}

std::optional<DeserializedRawMessage> deserializeBufferToMessage(network::ByteBuffer buffer) {
    cbor_load_result load_result{};
    auto msg_array = detail::bufferToCbor(std::move(buffer), &load_result);

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
    auto msg_type_item = detail::CborPtr(cbor_array_get(msg_array.get(), 0));
    if (!msg_type_item || !cbor_isa_uint(msg_type_item.get())) {
        LOG_ERROR("Message type in cbor is not unsigned integer");
        return std::nullopt;
    }

    auto packed_opt = detail::readCborUint(msg_type_item.get());
    if (!packed_opt) {
        LOG_ERROR("Failed to read message type value");
        return std::nullopt;
    }
    auto packed_type = static_cast<uint16_t>(*packed_opt);

    auto msg_type = types::unpackMessageType(packed_type);
    if (!msg_type) {
        LOG_ERROR(std::format("Failed to unpack message type from {}", packed_type));
        return std::nullopt;
    }

    auto msg_args = detail::getMsgArgsFromCborArray(msg_array.get());
    if (!msg_args) {
        LOG_ERROR("Failed to extract arguments from CBOR array");
        return std::nullopt;
    }

    return DeserializedRawMessage{*msg_type, *msg_args};
}

}  // namespace dungeons::common::wire
