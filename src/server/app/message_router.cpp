#include "message_router.h"

#include <common/types/message_types.h>
#include <common/types/strong_id_format.h>
#include <common/utility/logger.h>
#include <common/wire/serder.h>
#include <server/app/message_dispatcher.h>

#include "session_registry.h"

namespace dungeons::server::app {

std::shared_ptr<MessageRouter> MessageRouter::create(std::shared_ptr<core::EventBus> event_bus,
                                                     std::shared_ptr<SessionRegistry> session_registry) {
    struct EnableMakeShared : MessageRouter {
        EnableMakeShared(std::shared_ptr<core::EventBus> bus, std::shared_ptr<SessionRegistry> reg)
            : MessageRouter(std::move(bus), std::move(reg)) {}
    };

    auto router = std::make_shared<EnableMakeShared>(std::move(event_bus), std::move(session_registry));
    router->Initialize();

    return router;
}

MessageRouter::MessageRouter(std::shared_ptr<core::EventBus> event_bus,
                             std::shared_ptr<SessionRegistry> session_registry)
    : event_bus_{std::move(event_bus)}
    , session_registry_{std::move(session_registry)} {}

void MessageRouter::Initialize() {
    connection_with_event_bus_ =
        event_bus_->subscribe<network::RawMessageReceivedEvent>([self = shared_from_this()](const auto& event) {
            self->OnRawMessage(event);
        });
}

void MessageRouter::OnRawMessage(const network::RawMessageReceivedEvent& event) {
    auto sid = event.session_id.get();
    LOG_INFO(std::format("[MessageRouter: Session #{}] processing RawMessageReceivedEvent, included message size={}",
                         sid,
                         event.message.buffer.size()));

    auto message = common::wire::deserializeRawMessage(event.message.buffer);

    if (!message) {
        LOG_ERROR("Failed to deserialize message");
        return;
    }

    LOG_INFO(std::format("[MessageRouter: Session #{}] message deserialized with args number={}",
                         sid,
                         message->args.size()));

    common::wire::MessageArgs args{std::in_place};

    using namespace dungeons::common::types;

    if (isNetworkMessage(message->type)) {
        LOG_INFO(std::format("[MessageRouter: Session #{}] message received is NetworkMessage", sid));
        auto type = asNetworkMessage(message->type);
        if (type.has_value() && (type == NetworkMessageType::kJoin || type == NetworkMessageType::kReconnect)) {
            LOG_INFO(std::format("[MessageRouter] adding sessionId #{} to event args", sid));
            args->emplace_back(event.session_id.get());
        }
    }

    if (isAppMessage(message->type)) {
        LOG_INFO(std::format("[MessageRouter: Session #{}] message received is AppMessage", sid));
        auto type = asAppMessage(message->type);
        if (type.has_value() && (type == AppMessageType::kJoinParty || type == AppMessageType::kLeaveParty ||
                                 type == AppMessageType::kCreateParty || type == AppMessageType::kListParties ||
                                 type == AppMessageType::kStartGame)) {
            if (auto pid = checkPlayer(event.session_id)) {
                LOG_INFO(std::format("[MessageRouter] adding playerId #{} to event args", *pid));
                args->emplace_back(pid.value());
            } else {
                LOG_ERROR(std::format("[MessageRouter] cant find Player connected with Session #{}", sid));
            }
        }
    }

    if (isDomainMessage(message->type)) {
        LOG_INFO(std::format("[MessageRouter: Session #{}] message received is DomainMessage", sid));
        auto type = asDomainMessage(message->type);
        if (type.has_value() && (type == DomainMessageType::kMove || type == DomainMessageType::kAttack)) {
            if (auto pid = checkPlayer(event.session_id)) {
                LOG_INFO(std::format("[MessageRouter] adding playerId #{} to event args.", *pid));
                args->emplace_back(pid.value());
            } else {
                LOG_ERROR(std::format("[MessageRouter] cant find Player connected with Session #{}", sid));
            }
        }
    }

    std::ranges::move(message->args, std::back_inserter(args.value()));
    auto event_from_message = makeEvent(message->type, args);

    if (!event_from_message) {
        LOG_ERROR("Failed to create event");
        return;
    }

    LOG_INFO(std::format("[MessageRouter: Session #{}] publishing event {} with args number={}",
                         sid,
                         core::eventTypeToString(event_from_message->getType()),
                         args->size()));

    event_bus_->publish(*event_from_message);
}
std::optional<domain::PlayerId> MessageRouter::checkPlayer(network::SessionId session_id) const {
    auto pid = session_registry_->getPlayerIdBySessionId(session_id);
    if (!pid.has_value()) {
        LOG_ERROR("Failed to get player id");
        return std::nullopt;
    }
    return pid;
}

}  // namespace dungeons::server::app
