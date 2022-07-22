#include <FeCore/Math/VectorMath.h>
#include <FeCore/Memory/StackAllocator.h>
#include <gtest/gtest.h>

using FE::Vector3F;

TEST(StackAllocator, Basic)
{
    FE::FeStackAllocator alloc(sizeof(Vector3F) * 3);

    {
        auto* vec1 = alloc.Allocate<Vector3F>(1, 2, 3);
        auto* vec2 = alloc.Allocate<Vector3F>(4, 5, 6);
        auto* sum  = alloc.Allocate<Vector3F>(5, 7, 9);
        EXPECT_EQ(*vec1, Vector3F(1, 2, 3));
        EXPECT_EQ(*vec2, Vector3F(4, 5, 6));
        EXPECT_EQ(*vec1 + *vec2, *sum);
    }
    alloc.Reset();
    {
        auto* vec1 = alloc.Allocate<Vector3F>(1, 2, 3);
        auto* vec2 = alloc.Allocate<Vector3F>(4, 5, 6);
        auto* sum  = alloc.Allocate<Vector3F>(5, 7, 9);
        EXPECT_EQ(*vec1, Vector3F(1, 2, 3));
        EXPECT_EQ(*vec2, Vector3F(4, 5, 6));
        EXPECT_EQ(*vec1 + *vec2, *sum);
    }
}

TEST(StackAllocator, Array)
{
    FE::FeStackAllocator alloc(512 * sizeof(int));

    auto* array = alloc.AllocateArray<int>(512);

    for (int i = 0; i < 512; ++i)
        array[i] = i;
    for (int i = 0; i < 512; ++i)
        EXPECT_EQ(i, array[i]);

#if FE_DEBUG
    EXPECT_DEATH(alloc.Allocate<int>(0), ".*");
#elif FE_RELEASE
    // EXPECT_ANY_THROW(alloc.Allocate<int>(0));
#endif
}
