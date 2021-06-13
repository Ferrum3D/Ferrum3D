#pragma once
#include <FerrumCore/FeStackAllocator.h>
#include "FeTestFramework.h"
#include <FerrumCore/FeVector.h>

namespace Ferrum
{
	FE_TEST(Basic, StackAlloc)
	{
		FeStackAllocator alloc{ sizeof(float3) * 3 };
		
		{
			auto* vec1 = alloc.Allocate<float3>(1, 2, 3);
			auto* vec2 = alloc.Allocate<float3>(4, 5, 6);
			auto* sum = alloc.Allocate<float3>(5, 7, 9);
			FT_EXPECT(vec1->X == 1 && vec1->Y == 2 && vec1->Z == 3);
			FT_EXPECT(vec2->X == 4 && vec2->Y == 5 && vec2->Z == 6);
			FT_EXPECT(*vec1 + *vec2 == *sum);
		}
		alloc.Reset();
		{
			auto* vec1 = alloc.Allocate<float3>(1, 2, 3);
			auto* vec2 = alloc.Allocate<float3>(4, 5, 6);
			auto* sum = alloc.Allocate<float3>(5, 7, 9);
			FT_EXPECT(vec1->X == 1 && vec1->Y == 2 && vec1->Z == 3);
			FT_EXPECT(vec2->X == 4 && vec2->Y == 5 && vec2->Z == 6);
			FT_EXPECT(*vec1 + *vec2 == *sum);
		}
	}

	FE_TEST(Array, StackAlloc)
	{
		FeStackAllocator alloc{ 512 * sizeof(int) };

		auto* array = alloc.AllocateArray<int>(512);

		for (int i = 0; i < 512; ++i)
			array[i] = i;
		for (int i = 0; i < 512; ++i)
			FT_EXPECT(i == array[i]);

		// must debug break on overflow here:
		// alloc.Allocate<int>(0);
	}
}
