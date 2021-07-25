#include <FeCore/Math/FeVector3.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(Vector3F, Add)
{
    float3 a{ 1, 2, 3 };
    float3 b{ 1, 2, 3 };
    float3 c{ 2, 4, 6 };
    EXPECT_EQ(a + b, c);
}

TEST(Vector3F, Sub)
{
    float3 a{ 1, 2, 3 };
    float3 b{ 1, 2, 3 };
    float3 c{ 0, 0, 0 };
    EXPECT_EQ(a - b, c);
}

TEST(Vector3F, Cross)
{
    float3 a{ 1, 2, 3 };
    float3 b{ 3, 2, 1 };
    float3 c{ -4, 8, -4 };
    EXPECT_EQ(CrossProd(a, b), c);
}

TEST(Vector3F, Dot)
{
    float3 a{ 1, 2, 3 };
    float3 b{ 1, 2, 3 };
    EXPECT_EQ(DotProd(a, b), 14);
}
