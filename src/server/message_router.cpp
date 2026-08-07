#include "message_router.h"

#include <format>

#include "../common/logger.h"
#include "../common/message_dispatcher.h"

namespace message_router {

std::shared_ptr<MessageRouter> MessageRouter::create(std::shared_ptr<events::EventBus> event_bus,
                                                     std::shared_ptr<network::SessionRegistry> session_registry) {
    struct EnableMakeShared : MessageRouter {
        EnableMakeShared(std::shared_ptr<events::EventBus> bus, std::shared_ptr<network::SessionRegistry> reg) :
            MessageRouter(std::move(bus), std::move(reg)) {}
    };

    auto router = std::make_shared<EnableMakeShared>(std::move(event_bus), std::move(session_registry));
    router->Initialize();

    return router;
}

MessageRouter::MessageRouter(std::shared_ptr<events::EventBus> event_bus,
                             std::shared_ptr<network::SessionRegistry> session_registry) :
    event_bus_{std::move(event_bus)}, session_registry_{std::move(session_registry)} {}

void MessageRouter::Initialize() {
    connection_with_event_bus_ = event_bus_->subscribe<events::RawMessageReceivedEvent>(
        std::bind(&MessageRouter::OnRawMessage, shared_from_this(), std::placeholders::_1));
}

void MessageRouter::OnRawMessage(const events::RawMessageReceivedEvent& raw_message_received_event) {
    auto& sid = raw_message_received_event.session_id.value;
    LOG_INFO(std::format("[MessageRouter: Session #{}] processing RawMessageReceivedEvent, included message size={}",
                         sid,
                         raw_message_received_event.message.message_data.size()));

    auto message = serialization::deserializeMessageRaw(raw_message_received_event.message);

    if (!message) {
        LOG_ERROR("Failed to deserialize message");
        return;
    }

    LOG_INFO(std::format("[MessageRouter: Session #{}] message deserialized with args number={}",
                         sid,
                         message->args.size()));
    serialization::MessageArgs args{std::in_place};

    if (message::isNetworkMessage(message->type)) {
        LOG_INFO(std::format("[MessageRouter: Session #{}] message received is NetworkMessage", sid));
        auto type = message::asNetworkMessage(message->type);
        if (type.has_value() &&
            (type == message::NetworkMessageType::kJoin || type == message::NetworkMessageType::kReconnect)) {
            LOG_INFO(std::format("[MessageRouter] adding sessionId #{} to event args", sid));
            args->emplace_back(raw_message_received_event.session_id.value);
        }
    }

    if (message::isAppMessage(message->type)) {
        LOG_INFO(std::format("[MessageRouter: Session #{}] message received is AppMessage", sid));
        auto type = message::asAppMessage(message->type);
        if (type.has_value() &&
            (type == message::AppMessageType::kJoinParty || type == message::AppMessageType::kLeaveParty ||
             type == message::AppMessageType::kCreateParty || type == message::AppMessageType::kListParties ||
             type == message::AppMessageType::kStartGame)) {
            if (auto pid = checkPlayer(raw_message_received_event.session_id)) {
                LOG_INFO(std::format("[MessageRouter] adding playerId #{} to event args", pid->value));
                args->emplace_back(pid.value());
            } else {
                LOG_ERROR(std::format("[MessageRouter] cant find Player connected with Session #{}", sid));
            }
        }
    }

    if (message::isDomainMessage(message->type)) {
        LOG_INFO(std::format("[MessageRouter: Session #{}] message received is DomainMessage", sid));
        auto type = message::asDomainMessage(message->type);
        if (type.has_value() &&
            (type == message::DomainMessageType::kMove || type == message::DomainMessageType::kAttack)) {
            if (auto pid = checkPlayer(raw_message_received_event.session_id)) {
                LOG_INFO(std::format("[MessageRouter] adding playerId #{} to event args.", pid->value));
                args->emplace_back(pid.value());
            } else {
                LOG_ERROR(std::format("[MessageRouter] cant find Player connected with Session #{}", sid));
            }
        }
    }

    std::ranges::move(message->args, std::back_inserter(args.value()));
    auto event = message_dispatcher::makeEvent(message->type, args);

    if (!event) {
        LOG_ERROR("Failed to create event");
        return;
    }

    LOG_INFO(std::format("[MessageRouter: Session #{}] publishing event {} with args number={}",
                         sid,
                         events::eventTypeToString(event->getType()),
                         args->size()));

    event_bus_->publish(*event);
}
std::optional<PlayerId> MessageRouter::checkPlayer(SessionId session_id) const {
    auto pid = session_registry_->getPlayerIdBySessionId(session_id);
    if (!pid.has_value()) {
        LOG_ERROR("Failed to get player id");
        return std::nullopt;
    }
    return pid;
}

}  // namespace message_router
