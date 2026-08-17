#include <common/types/direction.h>
#include <gtest/gtest.h>
#include <memory>
#include <server/domain/dungeon/collision_check.h>
#include <server/domain/dungeon/entity_manager.h>
#include <server/domain/dungeon/monster_control.h>
#include <server/domain/map/game_map.h>
#include <vector>

using namespace dungeons::server::domain;
using namespace dungeons::common::types;

// ------------------------------------------------------------------
// Mock DirectionGenerator – returns a fixed sequence
// ------------------------------------------------------------------
class MockDirectionGenerator : public DirectionGenerator {
public:
    explicit MockDirectionGenerator(std::vector<Direction> dirs)
        : dirs_(std::move(dirs))
        , index_(0) {}

    Direction generate() const override {
        if (dirs_.empty())
            return Direction::kUp;
        auto dir = dirs_[index_ % dirs_.size()];
        ++index_;
        return dir;
    }

private:
    std::vector<Direction> dirs_;
    mutable size_t index_;
};

// ------------------------------------------------------------------
// Helpers to create entities – using default config
// ------------------------------------------------------------------
static PlayerEntity createPlayer(PlayerId id, Position pos) {
    return PlayerEntity(id, pos);
}

static MonsterEntity createMonster(MobId id, Position pos, uint32_t attackRadius = 1) {
    EntityData data = getCfgMonsterData();
    data.radius_attack = attackRadius;
    return MonsterEntity(id, pos, data);
}

// ------------------------------------------------------------------
// Test Fixture – sets up a 10x10 map with some barriers
// ------------------------------------------------------------------
class MonsterControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        map_ = std::make_unique<GameMap>(MapSize{Position{0, 0}, Position{9, 9}});
        map_->addBarrier(Position{4, 4});
        map_->addBarrier(Position{5, 5});
        map_->addBarrier(Position{1, 1});
    }

    std::unique_ptr<GameMap> map_;
};

// ================================================================
// Movement tests – deterministic, exact positions
// ================================================================

TEST_F(MonsterControllerTest, MoveToFreeCell) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{0, 0}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    controller.moveRandomMonsters(1);

    const auto* mon = monsters.getEntity(MobId{101});
    ASSERT_NE(mon, nullptr);
    EXPECT_EQ(mon->GetPosition(), Position(0, 1));
    EXPECT_TRUE(map_->isInMap(mon->GetPosition()));
    EXPECT_FALSE(map_->isBarrier(mon->GetPosition()));
}

TEST_F(MonsterControllerTest, MoveBlockedByBarriers) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    map_->addBarrier(Position{2, 3});
    map_->addBarrier(Position{2, 1});
    map_->addBarrier(Position{1, 2});
    map_->addBarrier(Position{3, 2});
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{2, 2}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    controller.moveRandomMonsters(1);

    const auto* mon = monsters.getEntity(MobId{101});
    ASSERT_NE(mon, nullptr);
    EXPECT_EQ(mon->GetPosition(), Position(2, 2));
}

TEST_F(MonsterControllerTest, MoveBlockedByPlayers) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{2, 2}));
    players.getEntities().emplace(PlayerId{1}, createPlayer(PlayerId{1}, Position{2, 3}));
    players.getEntities().emplace(PlayerId{2}, createPlayer(PlayerId{2}, Position{2, 1}));
    players.getEntities().emplace(PlayerId{3}, createPlayer(PlayerId{3}, Position{1, 2}));
    players.getEntities().emplace(PlayerId{4}, createPlayer(PlayerId{4}, Position{3, 2}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    controller.moveRandomMonsters(1);

    const auto* mon = monsters.getEntity(MobId{101});
    ASSERT_NE(mon, nullptr);
    EXPECT_EQ(mon->GetPosition(), Position(2, 2));
}

TEST_F(MonsterControllerTest, SkipDeadMonstersInMove) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{0, 0}));
    monsters.getEntities().emplace(MobId{102}, createMonster(MobId{102}, Position{1, 1}));
    auto* m102 = monsters.getEntity(MobId{102});
    m102->damage(1000);  // kill it

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    controller.moveRandomMonsters(1);

    const auto* m101 = monsters.getEntity(MobId{101});
    ASSERT_NE(m101, nullptr);

    const auto* m102_dead = monsters.getEntity(MobId{102});
    ASSERT_NE(m102_dead, nullptr);
    EXPECT_FALSE(m102_dead->isAlive());
    EXPECT_EQ(m102_dead->GetPosition(), Position(1, 1));
}

TEST_F(MonsterControllerTest, ZeroMoves) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{0, 0}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    controller.moveRandomMonsters(0);

    const auto* mon = monsters.getEntity(MobId{101});
    ASSERT_NE(mon, nullptr);
    EXPECT_EQ(mon->GetPosition(), Position(0, 0));
}

// ================================================================
// Attack tests – only check no crash / no throw
// ================================================================

TEST_F(MonsterControllerTest, AttackNoCrashWithMonstersAndPlayers) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;

    players.getEntities().emplace(PlayerId{1}, createPlayer(PlayerId{1}, Position{0, 0}));
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{1, 0}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    EXPECT_NO_THROW(controller.performRandomAttacks(10, 1));
}

TEST_F(MonsterControllerTest, AttackNoCrashWithNoMonsters) {
    EntityManager<MonsterEntity, MobId> emptyMonsters;
    EntityManager<PlayerEntity, PlayerId> players;
    players.getEntities().emplace(PlayerId{1}, createPlayer(PlayerId{1}, Position{0, 0}));

    CollisionChecker checker(*map_, players, emptyMonsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(emptyMonsters, players, checker, dirGen);

    EXPECT_NO_THROW(controller.performRandomAttacks(10, 5));
}

TEST_F(MonsterControllerTest, AttackNoCrashWithNoPlayers) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> emptyPlayers;
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{0, 0}));

    CollisionChecker checker(*map_, emptyPlayers, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, emptyPlayers, checker, dirGen);

    EXPECT_NO_THROW(controller.performRandomAttacks(10, 5));
}

// Also test that the function can be called with zero power – just to check it doesn't blow up.
TEST_F(MonsterControllerTest, AttackZeroPowerDoesNotCrash) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    players.getEntities().emplace(PlayerId{1}, createPlayer(PlayerId{1}, Position{0, 0}));
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{1, 0}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    EXPECT_NO_THROW(controller.performRandomAttacks(0, 1));
}

TEST_F(MonsterControllerTest, AttackMaxAttacksLargeDoesNotCrash) {
    EntityManager<MonsterEntity, MobId> monsters;
    EntityManager<PlayerEntity, PlayerId> players;
    players.getEntities().emplace(PlayerId{1}, createPlayer(PlayerId{1}, Position{0, 0}));
    monsters.getEntities().emplace(MobId{101}, createMonster(MobId{101}, Position{1, 0}));

    CollisionChecker checker(*map_, players, monsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(monsters, players, checker, dirGen);

    EXPECT_NO_THROW(controller.performRandomAttacks(10, 100));
}

// ================================================================
// Edge cases – empty managers for move
// ================================================================

TEST_F(MonsterControllerTest, MoveRandomMonstersEmpty) {
    EntityManager<MonsterEntity, MobId> emptyMonsters;
    EntityManager<PlayerEntity, PlayerId> players;
    CollisionChecker checker(*map_, players, emptyMonsters);
    MockDirectionGenerator dirGen({Direction::kUp});
    MonsterController controller(emptyMonsters, players, checker, dirGen);
    EXPECT_NO_THROW(controller.moveRandomMonsters(5));
}
