#include <C:/Users/qt96334/.conan2/p/b/gtestfde03c87b0d12/p/include/gtest/gtest.h>
#include <common/strong_id.h>
#include <optional>
#include <server/domain/dungeon/entity_manager.h>
#include <server/domain/map/position.h>

using namespace dungeons::server::domain;

// Specialize StrongIdHash for int so unordered_map works with int keys.
// This is only for testing; production uses StrongId types.
template <>
struct dungeons::common::StrongIdIdentityHash<int> {
    size_t operator()(const int& id) const noexcept {
        return std::hash<int>{}(id);
    }
};

struct TestEntity {
    TestEntity(int id, const Position& pos)
        : id_(id)
        , pos_(pos)
        , alive_(true) {}

    void SetPosition(const Position& pos) {
        pos_ = pos;
    }
    Position GetPosition() const {
        return pos_;
    }
    bool isAlive() const {
        return alive_;
    }
    void setAlive(bool alive) {
        alive_ = alive;
    }

    int id_;
    Position pos_;
    bool alive_ = true;
};

using TestManager = EntityManager<TestEntity, int>;

TEST(EntityManagerTest, DefaultConstruction) {
    TestManager mgr;
    EXPECT_TRUE(mgr.getEntities().empty());
}

TEST(EntityManagerTest, AddEntitySuccess) {
    TestManager mgr;
    Position pos(5, 10);
    EXPECT_TRUE(mgr.addEntity(42, pos));

    const auto* entity = mgr.getEntity(42);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->GetPosition(), pos);
    EXPECT_TRUE(entity->isAlive());
}

TEST(EntityManagerTest, AddEntityDuplicateFails) {
    TestManager mgr;
    Position pos(1, 2);
    EXPECT_TRUE(mgr.addEntity(100, pos));
    EXPECT_FALSE(mgr.addEntity(100, Position(3, 4)));  // duplicate id

    const auto* entity = mgr.getEntity(100);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->GetPosition(), pos);  // position unchanged
}

TEST(EntityManagerTest, GetEntityNonExistentReturnsNull) {
    TestManager mgr;
    EXPECT_EQ(mgr.getEntity(999), nullptr);
    const TestManager& constMgr = mgr;
    EXPECT_EQ(constMgr.getEntity(999), nullptr);
}

TEST(EntityManagerTest, IsOccupied) {
    TestManager mgr;
    Position pos1(0, 0);
    Position pos2(1, 1);
    mgr.addEntity(1, pos1);

    EXPECT_TRUE(mgr.isOccupied(pos1));
    EXPECT_FALSE(mgr.isOccupied(pos2));
}

TEST(EntityManagerTest, MoveEntity) {
    TestManager mgr;
    Position initial(2, 3);
    mgr.addEntity(7, initial);

    Position newPos(5, 6);
    EXPECT_TRUE(mgr.moveEntity(7, types::Direction::kUp, newPos));  // direction ignored, just uses newPos

    const auto* entity = mgr.getEntity(7);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->GetPosition(), newPos);
}

TEST(EntityManagerTest, MoveEntityNonExistentFails) {
    TestManager mgr;
    EXPECT_FALSE(mgr.moveEntity(99, types::Direction::kRight, Position(0, 0)));
}

TEST(EntityManagerTest, MoveEntityDeadFails) {
    TestManager mgr;
    Position pos(1, 1);
    mgr.addEntity(5, pos);
    auto* entity = mgr.getEntity(5);
    ASSERT_NE(entity, nullptr);
    entity->setAlive(false);

    EXPECT_FALSE(mgr.moveEntity(5, types::Direction::kUp, Position(2, 2)));

    // Position should not have changed.
    const auto* updated = mgr.getEntity(5);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->GetPosition(), pos);
}

TEST(EntityManagerTest, FindClosestTarget) {
    TestManager attackers;
    TestManager targets;

    // Add one attacker at (0,0)
    attackers.addEntity(1, Position(0, 0));

    // Add targets at various distances from (0,0)
    targets.addEntity(10, Position(2, 0));  // distance 2
    targets.addEntity(11, Position(0, 3));  // distance 3
    targets.addEntity(12, Position(1, 1));  // distance 2
    targets.addEntity(13, Position(5, 5));  // distance 10

    // Find closest within radius 3: should be either 10 or 12. The first one encountered is returned.
    // To make deterministic, add all, then check that the returned id is one with min distance.
    auto closest = attackers.findClosestTarget(Position(0, 0), 3, targets);
    ASSERT_TRUE(closest.has_value());
    int id = *closest;
    // The closest distance is 2, so the returned id must be 10 or 12.
    EXPECT_TRUE(id == 10 || id == 12);

    // Within radius 1: none in range.
    auto none = attackers.findClosestTarget(Position(0, 0), 1, targets);
    EXPECT_FALSE(none.has_value());
}

TEST(EntityManagerTest, FindClosestTargetIgnoresDeadTargets) {
    TestManager attackers;
    TestManager targets;

    attackers.addEntity(1, Position(0, 0));
    targets.addEntity(20, Position(1, 0));  // distance 1
    targets.addEntity(21, Position(0, 1));  // distance 1

    // Kill one target.
    auto* deadTarget = targets.getEntity(20);
    ASSERT_NE(deadTarget, nullptr);
    deadTarget->setAlive(false);

    auto closest = attackers.findClosestTarget(Position(0, 0), 2, targets);
    ASSERT_TRUE(closest.has_value());
    // The only alive target within radius is 21, so should return 21.
    EXPECT_EQ(*closest, 21);
}

TEST(EntityManagerTest, RemoveDeadEntities) {
    TestManager mgr;

    mgr.addEntity(1, Position(0, 0));
    mgr.addEntity(2, Position(1, 1));
    mgr.addEntity(3, Position(2, 2));

    auto* e2 = mgr.getEntity(2);
    ASSERT_NE(e2, nullptr);
    e2->setAlive(false);

    mgr.removeDeadEntities();

    EXPECT_EQ(mgr.getEntities().size(), 2);
    EXPECT_NE(mgr.getEntity(1), nullptr);
    EXPECT_EQ(mgr.getEntity(2), nullptr);
    EXPECT_NE(mgr.getEntity(3), nullptr);
}

TEST(EntityManagerTest, NonConstGetEntitiesForModification) {
    TestManager mgr;
    mgr.addEntity(1, Position(0, 0));

    auto& entities = mgr.getEntities();
    auto it = entities.find(1);
    ASSERT_NE(it, entities.end());
    it->second.SetPosition(Position(5, 5));

    const auto* entity = mgr.getEntity(1);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->GetPosition(), Position(5, 5));
}
