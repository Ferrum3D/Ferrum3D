#include "FeCore/ECS/ECS.h"
#include <gtest/gtest.h>
namespace FE::ECS
{
    TEST(ECS, Create)
    {
        Archetype archetype{};
        Entity entity = createEntity(archetype);

        EXPECT_EQ(archetype.m_chunks.size(), 1);
        ArchetypeChunk* chunk = archetype.m_chunks[0];
        EXPECT_EQ(chunk->entityCount, 1);
        EXPECT_TRUE(chunk->freeEntities[0]);

        deleteEntity(archetype, entity.Id);
        EXPECT_EQ(chunk->entityCount, 0);
        EXPECT_FALSE(chunk->freeEntities[0]);
    }
} // namespace FE::ECS
