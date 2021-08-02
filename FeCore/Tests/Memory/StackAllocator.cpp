#include <FeCore/Math/VectorMath.h>
#include <FeCore/Memory/StackAllocator.h>
#include <gtest/gtest.h>

using FE::float3;

TEST(StackAllocator, Basic)
{
    FE::FeStackAllocator alloc(sizeof(float3) * 3);

    {
        auto* vec1 = alloc.Allocate<float3>(1, 2, 3);
        auto* vec2 = alloc.Allocate<float3>(4, 5, 6);
        auto* sum  = alloc.Allocate<float3>(5, 7, 9);
        EXPECT_EQ(*vec1, float3(1, 2, 3));
        EXPECT_EQ(*vec2, float3(4, 5, 6));
        EXPECT_EQ(*vec1 + *vec2, *sum);
    }
    alloc.Reset();
    {
        auto* vec1 = alloc.Allocate<float3>(1, 2, 3);
        auto* vec2 = alloc.Allocate<float3>(4, 5, 6);
        auto* sum  = alloc.Allocate<float3>(5, 7, 9);
        EXPECT_EQ(*vec1, float3(1, 2, 3));
        EXPECT_EQ(*vec2, float3(4, 5, 6));
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
    EXPECT_ANY_THROW(alloc.Allocate<int>(0));
#endif
}
