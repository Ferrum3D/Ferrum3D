#include <FeCore/Components/PositionComponent.h>
#include <FeCore/ECS/EntityQuery.h>
#include <FeCore/ECS/EntityRegistry.h>
#include <gtest/gtest.h>

using namespace FE::ECS;
using FE::ArraySlice;
using FE::ArraySliceMut;
using FE::fe_typeid;
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
    auto entity   = registry->CreateEntity();
    registry->AddComponent(entity, PositionComponent(1, 2, 3));
    registry->AddComponent(entity, TestComponent{ 4 });
    auto clone = registry->CloneEntity(entity);
    EXPECT_EQ(registry->GetComponent<PositionComponent>(entity), registry->GetComponent<PositionComponent>(clone));
    EXPECT_EQ(registry->GetComponent<TestComponent>(entity).TestData, registry->GetComponent<TestComponent>(clone).TestData);
}

TEST(EntityRegistry, UpdateQuery)
{
    const int count = 16 * 1024;

    auto registry = FE::MakeShared<EntityRegistry>();
    List<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    registry->AddComponent<PositionComponent>(entities);
    registry->AddComponent<TestComponent>(ArraySlice(entities)(0, count / 2));

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        registry->SetComponent(entities[i],
                               PositionComponent(static_cast<float>(i), static_cast<float>(i) * 2, static_cast<float>(i) * 3));
        if (i < count / 2)
        {
            registry->SetComponent(entities[i], TestComponent{ static_cast<float>(i) * 10 });
        }
    }

    auto query1 = FE::MakeShared<EntityQuery>(registry.GetRaw());
    query1->AllOf<PositionComponent, TestComponent>().Update();

    auto query2 = FE::MakeShared<EntityQuery>(registry.GetRaw());
    query2->AllOf<PositionComponent>().Update();

    FE::USize entityCount1 = 0;
    FE::USize entityCount2 = 0;

    query1->ForEach(std::function([&entityCount1](PositionComponent& pos, TestComponent& test) {
        EXPECT_EQ(pos.Y, pos.X * 2);
        EXPECT_EQ(pos.Z, pos.X * 3);
        EXPECT_EQ(test.TestData, pos.X * 10);
        entityCount1++;
    }));

    query2->ForEach(std::function([&entityCount2](PositionComponent& pos) {
        EXPECT_EQ(pos.Y, pos.X * 2);
        EXPECT_EQ(pos.Z, pos.X * 3);
        entityCount2++;
    }));

    EXPECT_EQ(entityCount1, count / 2);
    EXPECT_EQ(entityCount2, count);
}

TEST(EntityRegistry, ReuseEntityIDs)
{
    const int count = 16 * 1024;

    auto registry = FE::MakeShared<EntityRegistry>();
    List<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    entities.SortByMember(&Entity::GetID);

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        EXPECT_EQ(entities[i].GetVersion(), 0);
        ASSERT_EQ(entities[i].GetID(), i);
    }

    registry->DestroyEntities(entities);
    registry->CreateEntities(ArraySliceMut(entities));
    entities.SortByMember(&Entity::GetID);

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        EXPECT_EQ(entities[i].GetVersion(), 1);
        ASSERT_EQ(entities[i].GetID(), i);
    }
}

TEST(EntityRegistry, HandleMultipleArchetypeChunks)
{
    // Create a lot of components, that do not fit into a single chunk
    const int count = 16 * 1024;

    auto registry = FE::MakeShared<EntityRegistry>();
    List<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    registry->AddComponent<PositionComponent>(entities);
    registry->AddComponent<TestComponent>(entities);

    for (FE::USize i = 0; i < entities.Size(); ++i)
    {
        registry->SetComponent(entities[i],
                               PositionComponent(static_cast<float>(i), static_cast<float>(i) * 2, static_cast<float>(i) * 3));
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
        EXPECT_EQ(static_cast<float>(i) * 2, registry->GetComponent<PositionComponent>(entities[i]).Y);
        ASSERT_EQ(static_cast<float>(i) * 3, registry->GetComponent<PositionComponent>(entities[i]).Z);
    }
}