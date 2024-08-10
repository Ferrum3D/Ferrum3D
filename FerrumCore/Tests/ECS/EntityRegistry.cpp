#if 0
#include <EASTL/sort.h>
#include <FeCore/Components/PositionComponent.h>
#include <FeCore/ECS/EntityQuery.h>
#include <FeCore/ECS/EntityRegistry.h>
#include <gtest/gtest.h>

using namespace FE::ECS;
using FE::ArraySlice;
using FE::ArraySliceMut;
using FE::fe_typeid;

struct TestComponent
{
    float TestData;

    FE_RTTI_Base(TestComponent, "DAAAA79F-A425-40A3-A308-49FC365E5437");

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
    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    eastl::vector<Entity> entities;
    entities.resize(64 * 1024, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
}

TEST(EntityRegistry, AddComponent)
{
    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    auto entity1 = registry->CreateEntity();
    auto entity2 = registry->CreateEntity();
    registry->AddComponent(entity1, Position3DComponent{ 1, 2, 99 });
    registry->AddComponent(entity1, TestComponent{ 9 });
    registry->AddComponent(entity2, Position3DComponent{ 3, 4, 99 });
    EXPECT_TRUE(registry->HasComponent<Position3DComponent>(entity1));
    EXPECT_TRUE(registry->HasComponent<TestComponent>(entity1));
    EXPECT_TRUE(registry->HasComponent<Position3DComponent>(entity2));
    EXPECT_EQ(registry->GetComponent<Position3DComponent>(entity1), (Position3DComponent{ 1, 2, 99 }));
    EXPECT_EQ(registry->GetComponent<TestComponent>(entity1), (TestComponent{ 9 }));
    EXPECT_EQ(registry->GetComponent<Position3DComponent>(entity2), (Position3DComponent{ 3, 4, 99 }));
}

TEST(EntityRegistry, InvalidAfterDestroy)
{
    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    auto entity1 = registry->CreateEntity();
    EXPECT_TRUE(registry->IsValid(entity1));
    registry->AddComponent(entity1, Position3DComponent{ 1, 2, 99 });
    registry->AddComponent(entity1, TestComponent{ 9 });
    registry->DestroyEntity(entity1);
    EXPECT_FALSE(registry->IsValid(entity1));
}

TEST(EntityRegistry, CloneEntity)
{
    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    auto entity = registry->CreateEntity();
    registry->AddComponent(entity, Position3DComponent(1, 2, 3));
    registry->AddComponent(entity, TestComponent{ 4 });
    auto clone = registry->CloneEntity(entity);
    EXPECT_EQ(registry->GetComponent<Position3DComponent>(entity), registry->GetComponent<Position3DComponent>(clone));
    EXPECT_EQ(registry->GetComponent<TestComponent>(entity).TestData, registry->GetComponent<TestComponent>(clone).TestData);
}

TEST(EntityRegistry, UpdateQuery)
{
    const int count = 16 * 1024;

    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    eastl::vector<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    registry->AddComponent<Position3DComponent>(entities);
    registry->AddComponent<TestComponent>(ArraySlice(entities)(0, count / 2));

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        registry->SetComponent(entities[i],
                               Position3DComponent(static_cast<float>(i), static_cast<float>(i) * 2, static_cast<float>(i) * 3));
        if (i < count / 2)
        {
            registry->SetComponent(entities[i], TestComponent{ static_cast<float>(i) * 10 });
        }
    }

    FE::Rc query1 = FE::Rc<EntityQuery>::DefaultNew(registry.Get());
    query1->AllOf<Position3DComponent, TestComponent>().Update();

    FE::Rc query2 = FE::Rc<EntityQuery>::DefaultNew(registry.Get());
    query2->AllOf<Position3DComponent>().Update();

    FE::size_t entityCount1 = 0;
    FE::size_t entityCount2 = 0;

    query1->ForEach(std::function([&entityCount1](Position3DComponent& pos, TestComponent& test) {
        EXPECT_EQ(pos.Y, pos.X * 2);
        EXPECT_EQ(pos.Z, pos.X * 3);
        EXPECT_EQ(test.TestData, pos.X * 10);
        entityCount1++;
    }));

    query2->ForEach(std::function([&entityCount2](Position3DComponent& pos) {
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

    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    eastl::vector<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    eastl::sort(entities.begin(), entities.end(), [](const Entity& lhs, const Entity& rhs) {
        return lhs.GetID() < rhs.GetID();
    });

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        EXPECT_EQ(entities[i].GetVersion(), 0);
        ASSERT_EQ(entities[i].GetID(), i);
    }

    registry->DestroyEntities(entities);
    registry->CreateEntities(ArraySliceMut(entities));
    eastl::sort(entities.begin(), entities.end(), [](const Entity& lhs, const Entity& rhs) {
        return lhs.GetID() < rhs.GetID();
    });

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        EXPECT_EQ(entities[i].GetVersion(), 1);
        ASSERT_EQ(entities[i].GetID(), i);
    }
}

TEST(EntityRegistry, HandleMultipleArchetypeChunks)
{
    // Create a lot of components, that do not fit into a single chunk
    const int count = 16 * 1024;

    FE::Rc registry = FE::Rc<EntityRegistry>::DefaultNew();
    eastl::vector<Entity> entities(count, Entity::Null());
    registry->CreateEntities(ArraySliceMut(entities));
    registry->AddComponent<Position3DComponent>(entities);
    registry->AddComponent<TestComponent>(entities);

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        registry->SetComponent(entities[i],
                               Position3DComponent(static_cast<float>(i), static_cast<float>(i) * 2, static_cast<float>(i) * 3));
        registry->SetComponent(entities[i], TestComponent{ static_cast<float>(i) * 10 });
    }

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        EXPECT_TRUE(registry->HasComponent<Position3DComponent>(entities[i]));
        EXPECT_TRUE(registry->HasComponent<TestComponent>(entities[i]));
        EXPECT_EQ(static_cast<float>(i) * 10, registry->GetComponent<TestComponent>(entities[i]).TestData);
        EXPECT_EQ(static_cast<float>(i), registry->GetComponent<Position3DComponent>(entities[i]).X);
        EXPECT_EQ(static_cast<float>(i) * 2, registry->GetComponent<Position3DComponent>(entities[i]).Y);
        ASSERT_EQ(static_cast<float>(i) * 3, registry->GetComponent<Position3DComponent>(entities[i]).Z);
    }

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        ASSERT_TRUE(registry->RemoveComponent<TestComponent>(entities[i]));
    }

    for (FE::size_t i = 0; i < entities.size(); ++i)
    {
        EXPECT_TRUE(registry->HasComponent<Position3DComponent>(entities[i]));
        EXPECT_FALSE(registry->HasComponent<TestComponent>(entities[i]));
        EXPECT_EQ(static_cast<float>(i), registry->GetComponent<Position3DComponent>(entities[i]).X);
        EXPECT_EQ(static_cast<float>(i) * 2, registry->GetComponent<Position3DComponent>(entities[i]).Y);
        ASSERT_EQ(static_cast<float>(i) * 3, registry->GetComponent<Position3DComponent>(entities[i]).Z);
    }
}
#endif
