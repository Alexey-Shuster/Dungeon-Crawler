#include "client_manager.h"

#include <format>

#include "utility/logger.h"

namespace network {

std::shared_ptr<ClientManager> ClientManager::create(boost::asio::io_context& io, const ClientConfig& config) {
    struct EnableMakeShared : ClientManager {
        EnableMakeShared(boost::asio::io_context& io_ref, const ClientConfig& config_ref) :
            ClientManager(io_ref, config_ref) {}
    };

    return std::make_shared<EnableMakeShared>(io, config);
}

ClientManager::ClientManager(boost::asio::io_context& io, const ClientConfig& config) :
    io_(io), config_(config), strand_(boost::asio::make_strand(io)), actions_global_enabled_(config.enable_actions),
    auth_global_enabled_(config.enable_auth) {
    clients_.reserve(config.client_count);
    for (size_t i = 0; i < config.client_count; ++i) {
        ManagedClient mc;
        mc.client = Client::create(io);
        mc.auth_enabled = config.enable_auth;
        mc.actions_enabled = config.enable_actions;
        mc.state = ManagedClient::Disconnected;
        clients_.push_back(std::move(mc));
    }
}

void ClientManager::start() {
    LOG_INFO("Starting manager...");
    boost::asio::post(strand_, [self = shared_from_this()]() {
        for (size_t i = 0; i < self->clients_.size(); ++i) {
            self->connectClient(i);
        }
    });
}

void ClientManager::stop() {
    LOG_INFO("Stopping manager...");
    boost::asio::post(strand_, [self = shared_from_this()]() {
        for (auto& mc : self->clients_) {
            if (mc.action_timer) {
                mc.action_timer->cancel();
                mc.action_timer.reset();
            }
            if (mc.auth_timer) {
                mc.auth_timer->cancel();
                mc.auth_timer.reset();
            }
            if (mc.client) {
                mc.client->handleDisconnect();  // forces close
            }
            mc.state = ManagedClient::Disconnected;
        }
    });
}

void ClientManager::setActionsEnabled(bool enabled) {
    boost::asio::post(strand_, [self = shared_from_this(), enabled]() {
        self->actions_global_enabled_ = enabled;
        if (!enabled) {
            // Stop all action timers
            for (auto& mc : self->clients_) {
                if (mc.action_timer) {
                    mc.action_timer->cancel();
                    mc.action_timer.reset();
                }
            }
        } else {
            // Restart sending for clients that are Ready
            for (size_t i = 0; i < self->clients_.size(); ++i) {
                auto& mc = self->clients_[i];
                if (mc.state == ManagedClient::Ready && mc.actions_enabled) {
                    self->startSending(i);
                }
            }
        }
    });
}

void ClientManager::setAuthEnabled(bool enabled) {
    boost::asio::post(strand_, [self = shared_from_this(), enabled]() {
        self->auth_global_enabled_ = enabled;
        // Note: already connected clients remain as they are.
        // For new connections (reconnects) we'll use this flag.
    });
}

void ClientManager::connectClient(size_t index) {
    auto& mc = clients_[index];
    if (mc.state != ManagedClient::Disconnected) {
        return;
    }
    mc.state = ManagedClient::Connecting;

    auto weakSelf = weak_from_this();
    auto client = mc.client;

    auto make_callback = [weakSelf, index](auto memberFunc) {
        return [weakSelf, index, memberFunc]<typename... T0>(T0&&... args) mutable {
            if (auto self = weakSelf.lock()) {
                boost::asio::post(self->strand_,
                                  [self, index, memberFunc, ... args = std::forward<T0>(args)]() mutable {
                                      (self.get()->*memberFunc)(index, std::move(args)...);
                                  });
            }
        };
    };

    client->setOnConnect(make_callback(&ClientManager::onConnected));
    client->setOnDisconnect(make_callback(&ClientManager::onDisconnected));
    client->setOnReceiveMessage(make_callback(&ClientManager::onMessage));

    LOG_INFO(std::format("[Manager] Starting client {} connect to {}:{}", index, config_.host, config_.port));
    client->startConnect(config_.host, static_cast<uint16_t>(config_.port));
}

void ClientManager::onConnected(size_t index) {
    auto& mc = clients_[index];
    if (mc.state == ManagedClient::Connecting) {
        LOG_INFO(std::format("[Manager] Client {} connected", index));
        // Auth?
        if (auth_global_enabled_ && mc.auth_enabled) {
            performAuth(index);
        } else {
            mc.state = ManagedClient::Ready;
            if (actions_global_enabled_ && mc.actions_enabled) {
                startSending(index);
            }
        }
    }
}

void ClientManager::onDisconnected(size_t index) {
    auto& mc = clients_[index];
    LOG_INFO(std::format("[Manager] Client {} disconnected", index));
    mc.state = ManagedClient::Disconnected;
    if (mc.action_timer) {
        mc.action_timer->cancel();
        mc.action_timer.reset();
    }
    if (mc.auth_timer) {
        mc.auth_timer->cancel();
        mc.auth_timer.reset();
    }
    if (config_.enable_reconnect && !mc.reconnect_pending) {
        scheduleReconnect(index);
    }
}

void ClientManager::onMessage(size_t index, const MessageData& msg) {
    auto& mc = clients_[index];
    // If we are in Authenticating state, treat as auth response
    if (mc.state == ManagedClient::Authenticating) {
        authResponse(index, msg);
        return;
    }
    // Otherwise, just log it (we don't need further processing)
    LOG_INFO(std::format("[Manager] Client {} received: {}", index, std::string(msg.begin(), msg.end())));
}

void ClientManager::performAuth(size_t index) {
    auto& mc = clients_[index];
    if (mc.state != ManagedClient::Ready && mc.state != ManagedClient::Connecting) {
        // Only from Connecting or Ready (if we re-auth)
        return;
    }
    LOG_INFO(std::format("[Manager] Client {} sending auth: {}", index, config_.auth_message));
    mc.client->send(MessageData(config_.auth_message.begin(), config_.auth_message.end()));
    mc.state = ManagedClient::Authenticating;

    // Set auth timeout
    mc.auth_timer = std::make_shared<boost::asio::steady_timer>(io_);
    mc.auth_timer->expires_after(config_.auth_timeout);
    mc.auth_timer->async_wait(
        boost::asio::bind_executor(strand_, [self = shared_from_this(), index](boost::system::error_code ec) {
            if (!ec) {
                self->onAuthTimeout(index);
            }
        }));
}

void ClientManager::onAuthTimeout(size_t index) {
    auto& mc = clients_[index];
    if (mc.state == ManagedClient::Authenticating) {
        LOG_ERROR(std::format("[Manager] Client {} auth timeout", index));
        // Treat as failure: disconnect and reconnect if enabled
        mc.client->handleDisconnect();  // will trigger onDisconnected
    }
}

void ClientManager::authResponse(size_t index, const MessageData& response) {
    auto& mc = clients_[index];
    if (mc.state != ManagedClient::Authenticating)
        return;

    // Cancel auth timer
    if (mc.auth_timer) {
        mc.auth_timer->cancel();
        mc.auth_timer.reset();
    }

    // For MVP we just log the response and assume success.
    // You could add checks based on response content.
    LOG_INFO(
        std::format("[Manager] Client {} auth response: {}", index, std::string(response.begin(), response.end())));
    mc.state = ManagedClient::Ready;

    if (actions_global_enabled_ && mc.actions_enabled) {
        startSending(index);
    }
}

void ClientManager::startSending(size_t index) {
    auto& mc = clients_[index];
    if (mc.state != ManagedClient::Ready)
        return;
    if (!actions_global_enabled_ || !mc.actions_enabled)
        return;
    if (config_.max_messages_per_client > 0 && mc.messages_sent >= config_.max_messages_per_client) {
        LOG_INFO(std::format("[Manager] Client {} reached max messages, stopping", index));
        return;
    }

    mc.action_timer = std::make_shared<boost::asio::steady_timer>(io_);
    doSend(index);
}

void ClientManager::doSend(size_t index) {
    auto& mc = clients_[index];
    if (mc.state != ManagedClient::Ready)
        return;
    if (!actions_global_enabled_ || !mc.actions_enabled)
        return;
    if (config_.max_messages_per_client > 0 && mc.messages_sent >= config_.max_messages_per_client) {
        return;
    }

    LOG_INFO(std::format("[Manager] Client {} sending: {}", index, config_.action_message));
    mc.client->send(MessageData(config_.action_message.begin(), config_.action_message.end()));
    mc.messages_sent++;

    // Schedule next send
    mc.action_timer->expires_after(config_.send_interval);
    mc.action_timer->async_wait(
        boost::asio::bind_executor(strand_, [self = shared_from_this(), index](boost::system::error_code ec) {
            if (!ec) {
                self->doSend(index);
            }
        }));
}

void ClientManager::scheduleReconnect(size_t index) {
    auto& mc = clients_[index];
    if (mc.reconnect_pending)
        return;
    mc.reconnect_pending = true;
    LOG_INFO(std::format("[Manager] Client {} will reconnect in {} ms", index, config_.reconnect_delay.count()));

    auto timer = std::make_shared<boost::asio::steady_timer>(io_);
    timer->expires_after(config_.reconnect_delay);
    timer->async_wait(
        boost::asio::bind_executor(strand_, [self = shared_from_this(), index, timer](boost::system::error_code ec) {
            if (!ec) {
                auto& mc2 = self->clients_[index];
                mc2.reconnect_pending = false;
                if (mc2.state == ManagedClient::Disconnected) {
                    self->connectClient(index);
                }
            }
        }));
}

}  // namespace network
