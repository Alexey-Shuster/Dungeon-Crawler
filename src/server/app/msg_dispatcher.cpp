#include "msg_dispatcher.h"

#include <common/types/message_utils.h>
#include <common/utility/logger.h>
#include <format>
#include <server/domain/core/types.h>
#include <unordered_map>

#include "events.h"
#include "msg_dispatcher_dtl.h"

namespace dungeons::server::app {

struct MessageTypeVariantHash {
    size_t operator()(const common::types::MessageTypeVariant& v) const {
        auto packed = packMessageType(v);
        return packed ? static_cast<size_t>(*packed) : 0;
    }
};

using EventFactory = std::function<std::shared_ptr<core::Event>(const common::wire::MessageArgs&)>;

/**
 * @brief Карта фабрик событий по типу сообщения.
 *
 * Каждая фабрика проверяет количество и корректность аргументов.
 * Если аргументы невалидны, возвращает nullptr.
 */
const std::unordered_map<dc_MsgVariant, EventFactory, MessageTypeVariantHash> kEventFactoryMap = {
    {dc_NetMsg::kJoin, detail::createEvent<AuthRequestedEvent, network::SessionId, domain::PlayerId>},

    {dc_NetMsg::kReconnect, detail::createEvent<ReconnectRequestedEvent, network::SessionId, domain::PlayerId>},

    {dc_AppMsg::kCreateParty, detail::createEvent<domain::CreateLobbyRequestEvent, domain::PlayerId>},

    {dc_AppMsg::kListParties, detail::createEvent<domain::ListLobbiesRequestEvent, domain::PlayerId>},

    {dc_AppMsg::kJoinParty, detail::createEvent<domain::JoinLobbyRequestEvent, domain::PlayerId, domain::LobbyId>},

    {dc_AppMsg::kLeaveParty, detail::createEvent<domain::LeaveLobbyRequestEvent, domain::PlayerId>},

    {dc_AppMsg::kStartGame, detail::createEvent<domain::StartGameRequestEvent, domain::PlayerId>},

    {dc_DmnMsg::kMove, detail::createEvent<domain::MoveRequestEvent, domain::PlayerId, common::types::Direction>},

    {dc_DmnMsg::kAttack, detail::createEvent<domain::AtackRequestEvent, domain::PlayerId>},
};

std::shared_ptr<core::Event> makeEvent(const dc_MsgVariant& msg_type, common::wire::MessageArgs msg_args) {
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
    return it->second(std::move(msg_args));
}

std::shared_ptr<core::Event> deserializeMessage(common::network::RawMessage message) {
    auto raw = common::wire::deserializeBufferToMessage(std::move(message.buffer));
    if (!raw) {
        LOG_ERROR("Failed to deserialize raw message");
        return nullptr;
    }
    return makeEvent(raw->type, raw->args);
}

}  // namespace dungeons::server::app
