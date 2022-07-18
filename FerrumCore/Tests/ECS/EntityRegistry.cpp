#include <FeCore/ECS/EntityRegistry.h>
#include <gtest/gtest.h>

using namespace FE::ECS;
using FE::ArraySlice;
using FE::ArraySliceMut;
using FE::List;

struct TestPositionComponent
{
    float X;
    float Y;

    FE_STRUCT_RTTI(TestPositionComponent, "DAAAA79F-A425-40A3-A308-49FC365E5437");

    friend bool operator==(const TestPositionComponent& lhs, const TestPositionComponent& rhs)
    {
        return lhs.X == rhs.X && lhs.Y == rhs.Y;
    }

    friend bool operator!=(const TestPositionComponent& lhs, const TestPositionComponent& rhs)
    {
        return !(rhs == lhs);
    }
};

struct TestRotationComponent
{
    float R;

    FE_STRUCT_RTTI(TestRotationComponent, "788EDA9D-F739-4BB5-BA3E-0013844840DC");

    friend bool operator==(const TestRotationComponent& lhs, const TestRotationComponent& rhs)
    {
        return lhs.R == rhs.R;
    }

    friend bool operator!=(const TestRotationComponent& lhs, const TestRotationComponent& rhs)
    {
        return !(rhs == lhs);
    }
};

TEST(EntityRegistry, CreateEntity)
{
    auto registry = FE::MakeShared<EntityRegistry>();
    List<Entity> entities;
    entities.Resize(1024, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
}

TEST(EntityRegistry, AddComponent)
{
    auto registry = FE::MakeShared<EntityRegistry>();
    auto entity1  = registry->CreateEntity();
    auto entity2  = registry->CreateEntity();
    registry->AddComponent(entity1, TestPositionComponent{ 1, 2 });
    registry->AddComponent(entity1, TestRotationComponent{ 9 });
    registry->AddComponent(entity2, TestPositionComponent{ 3, 4 });
    EXPECT_TRUE(registry->HasComponent<TestPositionComponent>(entity1));
    EXPECT_TRUE(registry->HasComponent<TestRotationComponent>(entity1));
    EXPECT_TRUE(registry->HasComponent<TestPositionComponent>(entity2));
    EXPECT_EQ(registry->GetComponent<TestPositionComponent>(entity1), (TestPositionComponent{ 1, 2 }));
    EXPECT_EQ(registry->GetComponent<TestRotationComponent>(entity1), (TestRotationComponent{ 9 }));
    EXPECT_EQ(registry->GetComponent<TestPositionComponent>(entity2), (TestPositionComponent{ 3, 4 }));
}
