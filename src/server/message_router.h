#pragma once

#include <boost/signals2/signal.hpp>
#include <memory>

#include "../common/events.h"
#include "../infra/eventbus.h"
#include "session_registry.h"

namespace message_router {
class MessageRouter : public std::enable_shared_from_this<MessageRouter> {
public:
    [[nodiscard]] static std::shared_ptr<MessageRouter> create(
        std::shared_ptr<events::EventBus> event_bus,
        std::shared_ptr<network::SessionRegistry> session_registry);

protected:
    MessageRouter(std::shared_ptr<events::EventBus> event_bus,
                  std::shared_ptr<network::SessionRegistry> session_registry);

private:
    void Initialize();

    void OnRawMessage(const events::RawMessageReceivedEvent& raw_message_received_event);

private:
    boost::signals2::scoped_connection connection_with_event_bus_;
    std::shared_ptr<events::EventBus> event_bus_;
    std::shared_ptr<network::SessionRegistry> session_registry_;

    std::optional<PlayerId> checkPlayer(SessionId session_id) const;
};
}  // namespace message_router
