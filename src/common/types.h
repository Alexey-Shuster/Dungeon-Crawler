#pragma once

#include <compare>  // IWYU pragma: keep // operator<=>
#include <cstddef>  // IWYU pragma: keep // size_t
#include <cstdint>  // IWYU pragma: keep // uint64_t
#include <vector>

template <typename Tag>
struct StrongId {
    uint64_t value;

    explicit constexpr StrongId(uint64_t v) : value(v) {}

    // Default three‑way comparison (C++20)
    auto operator<=>(const StrongId&) const = default;

    // Need for serialization
    operator uint64_t() const {
        return value;
    }

    // Prefix increment
    constexpr StrongId& operator++() noexcept {
        ++value;
        return *this;
    }

    // Postfix increment
    constexpr StrongId operator++(int) noexcept {
        StrongId tmp = *this;
        ++value;
        return tmp;
    }
};

struct SessionIdTag {};
struct PlayerIdTag {};
struct MobIdTag {};
struct LobbyIdTag {};
struct PartyIdTag {};
struct GameIdTag {};
struct EntityIdTag {};

using SessionId = StrongId<SessionIdTag>;
using PlayerId = StrongId<PlayerIdTag>;
using MobId = StrongId<MobIdTag>;
using LobbyId = StrongId<LobbyIdTag>;
using PartyId = StrongId<PartyIdTag>;
using GameId = StrongId<GameIdTag>;
using EntityId = StrongId<EntityIdTag>;

template <typename T>
struct StrongIdHash {
    size_t operator()(const T& id) const noexcept {
        // Simple mix to avoid clustering on 32‑bit platforms
        uint64_t v = id.value;
        return static_cast<size_t>(v ^ (v >> 32));
    }
};

// TODO: dummy struct - possibly used later in Party
struct PartyList {
    std::vector<PartyId> data;
};
