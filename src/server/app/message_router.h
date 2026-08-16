#pragma once

#include <boost/signals2/signal.hpp>
#include <memory>
#include <server/core/event_bus.h>
#include <server/domain/core/types.h>
#include <server/network/events.h>

#include "session_registry.h"

namespace dungeons::server::app {

class MessageRouter : public std::enable_shared_from_this<MessageRouter> {
public:
    [[nodiscard]] static std::shared_ptr<MessageRouter> create(std::shared_ptr<core::EventBus> event_bus,
                                                               std::shared_ptr<SessionRegistry> session_registry);

protected:
    MessageRouter(std::shared_ptr<core::EventBus> event_bus, std::shared_ptr<SessionRegistry> session_registry);

private:
    void Initialize();

    void OnRawMessage(const network::RawMessageReceivedEvent& event);

private:
    boost::signals2::scoped_connection connection_with_event_bus_;
    std::shared_ptr<core::EventBus> event_bus_;
    std::shared_ptr<SessionRegistry> session_registry_;

    std::optional<domain::PlayerId> checkPlayer(network::SessionId session_id) const;
};

}  // namespace dungeons::server::app
