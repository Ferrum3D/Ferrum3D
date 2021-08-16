#include <FeCore/Math/Vector3.h>
#include <array>
#include <gtest/gtest.h>
#include <random>

using FE::float3;

TEST(Float3, GetXYZ)
{
    float3 f{ 1, 2, 3 };
    EXPECT_EQ(f.X(), 1);
    EXPECT_EQ(f.Y(), 2);
    EXPECT_EQ(f.Z(), 3);

    EXPECT_EQ(f.X(), f[0]);
    EXPECT_EQ(f.Y(), f[1]);
    EXPECT_EQ(f.Z(), f[2]);

    EXPECT_EQ(f.X(), f(0));
    EXPECT_EQ(f.Y(), f(1));
    EXPECT_EQ(f.Z(), f(2));
}

TEST(Float3, SetXYZ)
{
    float3 f;
    f.Set(1, 2, 3);
    EXPECT_EQ(f.X(), 1);
    EXPECT_EQ(f.Y(), 2);
    EXPECT_EQ(f.Z(), 3);
    f.Set(-1, -1, -1);
    EXPECT_EQ(f.X(), -1);
    EXPECT_EQ(f.Y(), -1);
    EXPECT_EQ(f.Z(), -1);
    f.X() = 1;
    f.Y() = 2;
    f.Z() = 3;
    EXPECT_EQ(f.X(), 1);
    EXPECT_EQ(f.Y(), 2);
    EXPECT_EQ(f.Z(), 3);
}

TEST(Float3, Constants)
{
    EXPECT_EQ(float3::GetZero(), float3(0));

    EXPECT_EQ(float3::GetUnitX(), float3( 1, 0, 0 ));
    EXPECT_EQ(float3::GetUnitY(), float3( 0, 1, 0 ));
    EXPECT_EQ(float3::GetUnitZ(), float3( 0, 0, 1 ));
}

TEST(Float3, Lerp)
{
    float3 a{ 0, 5, 10 };
    float3 b{ 10, 25, 40 };
    EXPECT_EQ(a.Lerp(b, 0.5f), float3(5, 15, 25));
}

TEST(Float3, Add)
{
    float3 a{ 1, 2, 3 };
    float3 b{ 1, 2, 3 };
    float3 c{ 2, 4, 6 };
    EXPECT_EQ(a + b, c);
}

TEST(Float3, Sub)
{
    float3 a{ 1, 2, 3 };
    float3 b{ 1, 2, 3 };
    float3 c{ 0, 0, 0 };
    EXPECT_EQ(a - b, c);
}

TEST(Float3, Cross)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    auto CrossRef = [](const float* lhs, const float* rhs) -> std::array<float, 3> {
        // non-SIMD version for testing
        auto x = lhs[1] * rhs[2] - rhs[1] * lhs[2];
        auto y = lhs[2] * rhs[0] - rhs[2] * lhs[0];
        auto z = lhs[0] * rhs[1] - rhs[0] * lhs[1];
        return { x, y, z };
    };

    for (int i = 0; i < 100'000; ++i)
    {
        float3 a{ dist(mt), dist(mt), dist(mt) };
        float3 b{ dist(mt), dist(mt), dist(mt) };
        auto ref = FE::Vector3F(CrossRef(a.Data(), b.Data()));
        ASSERT_TRUE(a.Cross(b).IsApproxEqualTo(ref));
    }
}

TEST(Float3, Length)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    auto LenRef = [](const float* lhs) {
        // non-SIMD version for testing
        return lhs[0] * lhs[0] + lhs[1] * lhs[1] + lhs[2] * lhs[2];
    };

    for (int i = 0; i < 100'000; ++i)
    {
        float3 a{ dist(mt), dist(mt), dist(mt) };
        ASSERT_EQ(a.LengthSq(), LenRef(a.Data()));
        ASSERT_EQ(a.Length(), std::sqrt(LenRef(a.Data())));
    }
}

TEST(Float3, Dot)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    auto DotRef = [](const float* lhs, const float* rhs) {
        // non-SIMD version for testing
        return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
    };

    for (int i = 0; i < 100'000; ++i)
    {
        float3 a{ dist(mt), dist(mt), dist(mt) };
        float3 b{ dist(mt), dist(mt), dist(mt) };
        ASSERT_EQ(a.Dot(b), DotRef(a.Data(), b.Data()));
    }
}
