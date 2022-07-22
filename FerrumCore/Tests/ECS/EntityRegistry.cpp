#include <FeCore/Components/PositionComponent.h>
#include <FeCore/ECS/EntityRegistry.h>
#include <gtest/gtest.h>

using namespace FE::ECS;
using FE::fe_typeid;
using FE::ArraySlice;
using FE::ArraySliceMut;
using FE::List;

struct TestComponent
{
    float TestData;

    FE_STRUCT_RTTI(TestComponent, "DAAAA79F-A425-40A3-A308-49FC365E5437");

    friend bool operator==(const TestComponent& lhs, const TestComponent& rhs)
    {
        return lhs.TestData == lhs.TestData;
    }

    friend bool operator!=(const TestComponent& lhs, const TestComponent& rhs)
    {
        return !(rhs == lhs);
    }
};

TEST(EntityRegistry, CreateEntity)
{
    auto registry = FE::MakeShared<EntityRegistry>();
    List<Entity> entities;
    entities.Resize(64 * 1024, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
}

TEST(EntityRegistry, AddComponent)
{
    auto registry = FE::MakeShared<EntityRegistry>();
    auto entity1  = registry->CreateEntity();
    auto entity2  = registry->CreateEntity();
    registry->AddComponent(entity1, PositionComponent{ 1, 2 });
    registry->AddComponent(entity1, TestComponent{ 9 });
    registry->AddComponent(entity2, PositionComponent{ 3, 4 });
    EXPECT_TRUE(registry->HasComponent<PositionComponent>(entity1));
    EXPECT_TRUE(registry->HasComponent<TestComponent>(entity1));
    EXPECT_TRUE(registry->HasComponent<PositionComponent>(entity2));
    EXPECT_EQ(registry->GetComponent<PositionComponent>(entity1), (PositionComponent{ 1, 2 }));
    EXPECT_EQ(registry->GetComponent<TestComponent>(entity1), (TestComponent{ 9 }));
    EXPECT_EQ(registry->GetComponent<PositionComponent>(entity2), (PositionComponent{ 3, 4 }));
}

TEST(EntityRegistry, InvalidAfterDestroy)
{
    auto registry = FE::MakeShared<EntityRegistry>();
    auto entity1  = registry->CreateEntity();
    EXPECT_TRUE(registry->IsValid(entity1));
    registry->AddComponent(entity1, PositionComponent{ 1, 2 });
    registry->AddComponent(entity1, TestComponent{ 9 });
    registry->DestroyEntity(entity1);
    EXPECT_FALSE(registry->IsValid(entity1));
}

TEST(EntityRegistry, CloneEntity)
{
    auto registry = FE::MakeShared<EntityRegistry>();
    auto entity = registry->CreateEntity();
    registry->AddComponent(entity, PositionComponent(1, 2, 3));
    registry->AddComponent(entity, TestComponent { 4 });
    auto clone = registry->CloneEntity(entity);
    EXPECT_EQ(registry->GetComponent<PositionComponent>(entity), registry->GetComponent<PositionComponent>(clone));
    EXPECT_EQ(registry->GetComponent<TestComponent>(entity).TestData, registry->GetComponent<TestComponent>(clone).TestData);
}

TEST(EntityRegistry, HandleMultipleArchetypeChunks)
{
    // Create a lot of components, that do not fit into a single chunk
    const int count = 16 * 1024;

    [[maybe_unused]] const FE::TypeID& positionType = fe_typeid<PositionComponent>();
    [[maybe_unused]] const FE::TypeID& testType = fe_typeid<TestComponent>();

    auto registry = FE::MakeShared<EntityRegistry>();
    List<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    registry->AddComponent<PositionComponent>(entities);
    registry->AddComponent<TestComponent>(entities);

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        registry->SetComponent(entities[i], PositionComponent(static_cast<float>(i), static_cast<float>(i) * 2, static_cast<float>(i) * 3));
        registry->SetComponent(entities[i], TestComponent{ static_cast<float>(i) * 10 });
    }

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        EXPECT_TRUE(registry->HasComponent<PositionComponent>(entities[i]));
        EXPECT_TRUE(registry->HasComponent<TestComponent>(entities[i]));
        EXPECT_EQ(static_cast<float>(i) * 10, registry->GetComponent<TestComponent>(entities[i]).TestData);
        EXPECT_EQ(static_cast<float>(i), registry->GetComponent<PositionComponent>(entities[i]).X);
        EXPECT_EQ(static_cast<float>(i) * 2, registry->GetComponent<PositionComponent>(entities[i]).Y);
        ASSERT_EQ(static_cast<float>(i) * 3, registry->GetComponent<PositionComponent>(entities[i]).Z);
    }

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        ASSERT_TRUE(registry->RemoveComponent<TestComponent>(entities[i]));
    }

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        EXPECT_TRUE(registry->HasComponent<PositionComponent>(entities[i]));
        EXPECT_FALSE(registry->HasComponent<TestComponent>(entities[i]));
        EXPECT_EQ(static_cast<float>(i), registry->GetComponent<PositionComponent>(entities[i]).X);
//        EXPECT_EQ(static_cast<float>(i) * 2, registry->GetComponent<PositionComponent>(entities[i]).Y);
//        EXPECT_EQ(static_cast<float>(i) * 3, registry->GetComponent<PositionComponent>(entities[i]).Z);
    }
}
