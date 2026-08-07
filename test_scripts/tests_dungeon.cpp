#include <gtest/gtest.h>
#include <memory>

#include "../src/common/logger.h"
#include "../src/domain/dungeon/dungeon.h"

using namespace dungeon;

// Тестовый класс для доступа к protected полям
class TestableDungeon : public Dungeon {
public:
    template <typename PlayersContainer>
    explicit TestableDungeon(map::GameMap game_map, PlayersContainer&& container) :
        Dungeon(std::move(game_map), std::forward<PlayersContainer>(container)) {}

    using Dungeon::addMonsterEntity;
    using Dungeon::addPlayerEntity;
    using Dungeon::game_map_;
    using Dungeon::isAvailable;
    using Dungeon::monsters_entities_;
    using Dungeon::players_entities_;

    std::optional<entity::PlayerEntity> findPlayer(PlayerId id) const {
        auto it = players_entities_.find(id);
        if (it != players_entities_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<entity::MonsterEntity> findMonster(MobId id) const {
        auto it = monsters_entities_.find(id);
        if (it != monsters_entities_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool removePlayer(PlayerId id) {
        auto it = players_entities_.find(id);
        if (it != players_entities_.end()) {
            players_entities_.erase(it);
            return true;
        }
        return false;
    }

    bool isPositionFree(const map::Position& pos) const {
        return isAvailable(pos);
    }
};

TEST(DungeonTest, AddPlayer) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    PlayerId p1{1};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{});

    bool added = d->addPlayerEntity(p1);
    EXPECT_TRUE(added);

    auto found = d->findPlayer(p1);
    ASSERT_TRUE(found.has_value());

    EXPECT_FALSE(d->isPositionFree(found.value().GetPosition()));
}

TEST(DungeonTest, RemovePlayer) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    PlayerId p1{1};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{p1});

    auto pos = d->findPlayer(p1).value().GetPosition();
    bool removed = d->removePlayer(p1);
    EXPECT_TRUE(removed);

    auto found = d->findPlayer(p1);
    EXPECT_FALSE(found.has_value());

    EXPECT_TRUE(d->isPositionFree(pos));
}

TEST(DungeonTest, AddMonster) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    MobId m1{100};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{});

    bool added = d->addMonsterEntity(m1);
    EXPECT_TRUE(added);

    auto found = d->findMonster(m1);
    ASSERT_TRUE(found.has_value());

    EXPECT_FALSE(d->isPositionFree(found.value().GetPosition()));
}

TEST(DungeonTest, PositionOccupied) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    PlayerId p1{1};
    MobId m1{100};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{});

    d->addPlayerEntity(p1);
    auto player_pos = d->findPlayer(p1).value().GetPosition();

    EXPECT_FALSE(d->isPositionFree(player_pos));

    bool monsterAdded = d->addMonsterEntity(m1);
    EXPECT_TRUE(monsterAdded);
}

TEST(DungeonTest, OutOfBounds) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{5, 5}});
    PlayerId p1{1};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{});

    bool added = d->addPlayerEntity(p1);
    EXPECT_TRUE(added);

    auto found = d->findPlayer(p1);
    ASSERT_TRUE(found.has_value());
    auto pos = found.value().GetPosition();

    const auto& map_size = d->game_map_.size();
    EXPECT_GE(pos.x, map_size.getBottomLeftCorner().x);
    EXPECT_LE(pos.x, map_size.getTopRightCorner().x);
    EXPECT_GE(pos.y, map_size.getBottomLeftCorner().y);
    EXPECT_LE(pos.y, map_size.getTopRightCorner().y);
}

TEST(DungeonTest, PlayerMoveCommand) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    PlayerId p1{1};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{p1});

    auto old_pos = d->findPlayer(p1).value().GetPosition();

    d->addMovePlayerCommand(p1, map::Direction::kUp);
    d->processTick(std::chrono::milliseconds{16});

    auto new_pos = d->findPlayer(p1).value().GetPosition();
    EXPECT_NE(old_pos, new_pos);
}

TEST(DungeonTest, PlayerAttackCommand) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    PlayerId p1{1};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), std::vector<PlayerId>{p1});

    d->addPlayerAttackCommand(p1, 10);
    d->processTick(std::chrono::milliseconds{16});
    SUCCEED();
}

TEST(DungeonTest, ConstructorCreatesMonsters) {
    map::GameMap game_map(map::MapSize{map::Position{0, 0}, map::Position{20, 20}});
    auto players = std::vector<PlayerId>{PlayerId{1}, PlayerId{2}, PlayerId{3}};
    auto d = std::make_shared<TestableDungeon>(std::move(game_map), players);

    EXPECT_GT(d->monsters_entities_.size(), 0);
    EXPECT_EQ(d->monsters_entities_.size(), kMonstersPerPlayerCounter * players.size());
}
