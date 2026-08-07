#pragma once

#include "../../common/config.h"
#include "../../common/types.h"
#include "../map/position.h"
#include "entity_health.h"
#include "entity_types.h"

namespace entity {

struct EntityData {
    Health health;
    uint32_t radius_attack;
    uint32_t radius_view;
};

inline const auto& cfg = config::get_settings();

const EntityData kDefaultPlayerData{.health{Health(cfg.gameplay.player_default_hp)},
                                    .radius_attack = static_cast<uint32_t>(cfg.gameplay.default_radius_attack),
                                    .radius_view = static_cast<uint32_t>(cfg.gameplay.default_radius_view)};

const EntityData kDefaultMonsterData{.health{Health(cfg.gameplay.monster_default_hp)},
                                     .radius_attack = static_cast<uint32_t>(cfg.gameplay.default_radius_attack),
                                     .radius_view = static_cast<uint32_t>(cfg.gameplay.default_radius_view)};

class Entity {
public:
    Entity(map::Position position, EntityData data, EntityState state) :
        position_(position), data_(std::move(data)), state_(state) {}

    virtual ~Entity() = default;

    bool isAlive() const {
        return state_ == EntityState::Alive;
    }

    const map::Position& GetPosition() const {
        return position_;
    }

    map::Position& GetPosition() {
        return position_;
    }

    void SetPosition(const map::Position& position) {
        position_ = position;
    }

    void damage(HP damage) {
        data_.health -= damage;
        if (!data_.health) {
            SetState(EntityState::Dead);
        }
    }

    uint32_t getRadiusAttack() const noexcept {
        return data_.radius_attack;
    }

    uint32_t getRadiusView() const noexcept {
        return data_.radius_view;
    }

    Health getHealth() const noexcept {
        return data_.health;
    }

private:
    void SetState(EntityState state) {
        state_ = state;
    }

private:
    map::Position position_;
    EntityData data_;
    EntityState state_;
};

class PlayerEntity : public Entity {
public:
    PlayerEntity(PlayerId pid,
                 map::Position position,
                 EntityData data = kDefaultPlayerData,
                 EntityState state = EntityState::Alive) : Entity(position, std::move(data), state), pid_(pid) {}

    PlayerId GetId() const {
        return pid_;
    }

private:
    PlayerId pid_;
};

class MonsterEntity : public Entity {
public:
    MonsterEntity(MobId mid,
                  map::Position position,
                  EntityData data = kDefaultMonsterData,
                  EntityState state = EntityState::Alive) : Entity(position, std::move(data), state), mid_(mid) {}

    MobId GetId() const {
        return mid_;
    }

private:
    MobId mid_;
};

}  // namespace entity
