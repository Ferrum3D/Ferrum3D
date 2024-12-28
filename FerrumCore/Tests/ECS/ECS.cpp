#include "FeCore/ECS/ECS.h"
#include <gtest/gtest.h>

namespace FE::ECS
{
    struct PositionComponent
    {
        uint32_t x, y, z;
    };

    struct FlyComponent
    {
        int32_t gravity;
    };


    constexpr uint32_t componentSizes[] = { sizeof(PositionComponent), sizeof(FlyComponent) };

    TEST(ECS, CreateEntityFromRegistry)
    {
        EntityRegistry registry{};
        Entity entity = registry.CreateEntity<PositionComponent, FlyComponent>();
        Entity entity1 = registry.CreateEntity<PositionComponent>();

        EXPECT_EQ(registry.m_archetypes.size(), 2);
        EXPECT_EQ(registry.m_globalChunkTable[entity.m_chunkID]->m_allocatedEntityCount, 1);
        EXPECT_EQ(registry.m_globalChunkTable[entity1.m_chunkID]->m_allocatedEntityCount, 1);
    }

    TEST(ECS, CreateEntityFromArchetype)
    {
        EntityRegistry registry{};

        Rc<Archetype> archetype = Rc<Archetype>::DefaultNew(componentSizes);
        EXPECT_EQ(archetype->m_chunks.size(), 0);

        Entity entity = archetype->CreateEntity(registry.m_globalChunkTable);

        EXPECT_EQ(archetype->m_chunks.size(), 1);
        EXPECT_EQ(archetype->m_chunks[entity.m_chunkID]->m_allocatedEntityCount, 1);

        //EXPECT_EQ(registry.m_archetypes.size(), 1);
        EXPECT_EQ(registry.m_globalChunkTable[entity.m_chunkID]->m_allocatedEntityCount, 1);
    }

    TEST(ECS, DeleteEntityFromRegistry)
    {
        EntityRegistry registry{};
        Entity entity = registry.CreateEntity<PositionComponent, FlyComponent>();
        Rc<Archetype> archetype = Rc<Archetype>::DefaultNew(componentSizes);

        EXPECT_EQ(registry.m_archetypes.size(), 1);

        archetype->DestroyEntity(entity, registry.m_globalChunkTable);
        EXPECT_EQ(registry.m_globalChunkTable[entity.m_chunkID]->m_allocatedEntityCount, 0);
    }

} // namespace FE::ECS
