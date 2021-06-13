#pragma once
#include "FeTestFramework.h"
#include <FerrumCore/FeVector.h>

namespace Ferrum
{
	FE_TEST(Add, Vector3F, float3)
	{
		float3 a{ 1, 2, 3 };
		float3 b{ 1, 2, 3 };
		float3 c{ 2, 4, 6 };
		FT_EXPECT(a + b == c);
	}

	FE_TEST(Sub, Vector3F)
	{
		float3 a{ 1, 2, 3 };
		float3 b{ 1, 2, 3 };
		float3 c{ 0, 0, 0 };
		FT_EXPECT(a - b == c);
	}

	FE_TEST(Cross, Vector3F)
	{
		float3 a{ 1, 2, 3 };
		float3 b{ 3, 2, 1 };
		float3 c{ -4, 8, -4 };
		FT_EXPECT(CrossProd(a, b) == c);
	}

	FE_TEST(Dot, Vector3F)
	{
		float3 a{ 1, 2, 3 };
		float3 b{ 1, 2, 3 };
		FT_EXPECT(DotProd(a, b) == 14);
	}
}
