#include <FeCore/Math/Vector3.h>
#include <array>
#include <gtest/gtest.h>
#include <random>

using FE::Vector3F;

TEST(Vector3, GetXYZ)
{
    Vector3F f{ 1, 2, 3 };
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

TEST(Vector3, SetXYZ)
{
    Vector3F f;
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

TEST(Vector3, Constants)
{
    EXPECT_EQ(Vector3F::GetZero(), Vector3F(0));

    EXPECT_EQ(Vector3F::GetUnitX(), Vector3F(1, 0, 0));
    EXPECT_EQ(Vector3F::GetUnitY(), Vector3F(0, 1, 0));
    EXPECT_EQ(Vector3F::GetUnitZ(), Vector3F(0, 0, 1));
}

TEST(Vector3, Equal)
{
    EXPECT_EQ(Vector3F(1, 2, 3), Vector3F(1, 2, 3));

    EXPECT_NE(Vector3F(1, 2, 3), Vector3F(0, 2, 3));
    EXPECT_NE(Vector3F(1, 2, 3), Vector3F(1, 0, 3));
    EXPECT_NE(Vector3F(1, 2, 3), Vector3F(1, 2, 0));

    EXPECT_TRUE(Vector3F(1, 2, 3).IsApproxEqualTo(Vector3F(1.1f, 2.1f, 3.1f), 0.11f));

    EXPECT_FALSE(Vector3F(1, 2, 3).IsApproxEqualTo(Vector3F(1.2f, 2.1f, 3.1f), 0.11f));
    EXPECT_FALSE(Vector3F(1, 2, 3).IsApproxEqualTo(Vector3F(1.1f, 2.2f, 3.1f), 0.11f));
    EXPECT_FALSE(Vector3F(1, 2, 3).IsApproxEqualTo(Vector3F(1.1f, 2.1f, 3.2f), 0.11f));
}

TEST(Vector3, Lerp)
{
    Vector3F a{ 0, 5, 10 };
    Vector3F b{ 10, 25, 40 };
    EXPECT_EQ(a.Lerp(b, 0.5f), Vector3F(5, 15, 25));
}

TEST(Vector3, Add)
{
    Vector3F a{ 1, 2, 3 };
    Vector3F b{ 1, 2, 3 };
    Vector3F c{ 2, 4, 6 };
    EXPECT_EQ(a + b, c);
}

TEST(Vector3, Sub)
{
    Vector3F a{ 1, 2, 3 };
    Vector3F b{ 1, 2, 3 };
    Vector3F c{ 0, 0, 0 };
    EXPECT_EQ(a - b, c);
}

TEST(Vector3, Cross)
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
        Vector3F a{ dist(mt), dist(mt), dist(mt) };
        Vector3F b{ dist(mt), dist(mt), dist(mt) };
        auto ref = FE::Vector3F(CrossRef(a.Data(), b.Data()));
        ASSERT_TRUE(a.Cross(b).IsApproxEqualTo(ref));
    }
}

TEST(Vector3, MulEach)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    auto MulEachRef = [](const float* lhs, const float* rhs) -> std::array<float, 3> {
        // non-SIMD version for testing
        return { lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2] };
    };

    for (int i = 0; i < 100'000; ++i)
    {
        Vector3F a{ dist(mt), dist(mt), dist(mt) };
        Vector3F b{ dist(mt), dist(mt), dist(mt) };
        auto ref = FE::Vector3F(MulEachRef(a.Data(), b.Data()));
        ASSERT_TRUE(a.MulEach(b).IsApproxEqualTo(ref));
    }
}

TEST(Vector3, Length)
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
        Vector3F a{ dist(mt), dist(mt), dist(mt) };
        ASSERT_EQ(a.LengthSq(), LenRef(a.Data()));
        ASSERT_EQ(a.Length(), std::sqrt(LenRef(a.Data())));
    }
}

TEST(Vector3, Dot)
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
        Vector3F a{ dist(mt), dist(mt), dist(mt) };
        Vector3F b{ dist(mt), dist(mt), dist(mt) };
        ASSERT_EQ(a.Dot(b), DotRef(a.Data(), b.Data()));
    }
}

TEST(Vector3, Normalize)
{
    Vector3F a{1, 0, 0};
    EXPECT_TRUE(a.Normalized().IsApproxEqualTo(a));
    a.Set(1, 2, 3);
    EXPECT_TRUE(a.Normalized().IsApproxEqualTo(Vector3F(1.f / 3.7416573f, 2.f / 3.7416573f, 3.f / 3.7416573f)));
}
