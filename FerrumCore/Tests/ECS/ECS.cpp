#include "FeCore/ECS/ECS.h"
#include <gtest/gtest.h>

namespace FE::ECS
{
    TEST(ECS, Create)
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
        Rc<Archetype> archetype = Rc<Archetype>::DefaultNew(componentSizes);
        Entity entity1 = archetype->CreateEntity();

        EXPECT_EQ(archetype->m_chunks.size(), 1);
        auto& indices = archetype->m_chunks[0]->m_indexAllocator.m_freeIndices;

        bool hasOccupiedEntity = false;
        for (bool isFree : indices)
        {
            if (!isFree)
            {
                hasOccupiedEntity = true;
                break;
            }
        }
        EXPECT_TRUE(hasOccupiedEntity);

        archetype->DestroyEntity(entity1);
        bool isEmpty = true;
        for (bool isFree : indices)
        {
            if (!isFree)
            {
                isEmpty = false;
                break;
            }
        }
        EXPECT_TRUE(isEmpty);
    }
} // namespace FE::ECS
