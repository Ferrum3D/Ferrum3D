#include <FeCore/Math/Vector4.h>
#include <gtest/gtest.h>
#include <random>

using namespace FE;

TEST(Vector4, GetXYZW)
{
    const Vector4 vector{ 1, 2, 3, 4 };
    EXPECT_EQ(vector.x, 1);
    EXPECT_EQ(vector.y, 2);
    EXPECT_EQ(vector.z, 3);
    EXPECT_EQ(vector.w, 4);

    EXPECT_EQ(vector.Data()[0], 1);
    EXPECT_EQ(vector.Data()[1], 2);
    EXPECT_EQ(vector.Data()[2], 3);
    EXPECT_EQ(vector.Data()[3], 4);
}

TEST(Vector4, CreateFunctions)
{
    EXPECT_EQ(Vector4::Zero(), Vector4(0));

    EXPECT_EQ(Vector4::AxisX(), Vector4(1, 0, 0, 0));
    EXPECT_EQ(Vector4::AxisY(), Vector4(0, 1, 0, 0));
    EXPECT_EQ(Vector4::AxisZ(), Vector4(0, 0, 1, 0));
    EXPECT_EQ(Vector4::AxisW(), Vector4(0, 0, 0, 1));
}

TEST(Vector4, Load)
{
    const float alignas(__m128) values[] = { 1, 2, 3, 4, 5 };
    const Vector4 vector1 = Vector4::LoadAligned(values);
    EXPECT_EQ(vector1.x, 1);
    EXPECT_EQ(vector1.y, 2);
    EXPECT_EQ(vector1.z, 3);
    EXPECT_EQ(vector1.w, 4);

    const Vector4 vector2 = Vector4::LoadUnaligned(values + 1);
    EXPECT_EQ(vector2.x, 2);
    EXPECT_EQ(vector2.y, 3);
    EXPECT_EQ(vector2.z, 4);
    EXPECT_EQ(vector2.w, 5);
}

TEST(Vector4, Equality)
{
    EXPECT_EQ(Vector4(1, 2, 3, 4), Vector4(1, 2, 3, 4));

    EXPECT_NE(Vector4(1, 2, 3, 4), Vector4(0, 2, 3, 4));
    EXPECT_NE(Vector4(1, 2, 3, 4), Vector4(1, 0, 3, 4));
    EXPECT_NE(Vector4(1, 2, 3, 4), Vector4(1, 2, 0, 4));
    EXPECT_NE(Vector4(1, 2, 3, 4), Vector4(1, 2, 3, 0));

    EXPECT_TRUE(Math::EqualEstimate(Vector4(1, 2, 3, 4), Vector4(1.1f, 2.1f, 3.1f, 4.1f), 0.11f));
    EXPECT_TRUE(Math::EqualEstimate(Vector4(0.01f), Vector4::Zero(), 0.02f));
    EXPECT_TRUE(Math::EqualEstimate(Vector4::Zero(), Vector4(0.01f), 0.02f));
    EXPECT_TRUE(Math::EqualEstimate(Vector4(0.0f), Vector4(-0.0f)));

    EXPECT_FALSE(Math::EqualEstimate(Vector4(1, 2, 3, 4), Vector4(1.2f, 2.1f, 3.1f, 4.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector4(1, 2, 3, 4), Vector4(1.1f, 2.2f, 3.1f, 4.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector4(1, 2, 3, 4), Vector4(1.1f, 2.1f, 3.2f, 4.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector4(1, 2, 3, 4), Vector4(1.1f, 2.1f, 3.1f, 4.2f), 0.11f));
}

TEST(Vector4, Comparison)
{
    const Vector4 a{ 1, 2, 10, -1 };
    const Vector4 b{ 1, 5, 6, -1 };

    EXPECT_EQ(Math::CmpEqualMask(a, b), 0b1001);
    EXPECT_EQ(Math::CmpLessMask(a, b), 0b010);
    EXPECT_EQ(Math::CmpGreaterMask(a, b), 0b0100);
    EXPECT_EQ(Math::CmpLessEqualMask(a, b), 0b1011);
    EXPECT_EQ(Math::CmpGreaterEqualMask(a, b), 0b1101);
}

TEST(Vector4, Addition)
{
    const Vector4 a{ 1, 2, 3, -1 };
    const Vector4 b{ 1, 2, 3, 1 };
    const Vector4 c{ 2, 4, 6, 0 };

    EXPECT_TRUE(Math::EqualEstimate(a + b, c));
}

TEST(Vector4, Subtraction)
{
    const Vector4 a{ 1, 2, 3, 0 };
    const Vector4 b{ 1, 2, 3, 0 };
    const Vector4 c{ 0, 0, 0, 0 };

    EXPECT_TRUE(Math::EqualEstimate(a - b, c));
}

TEST(Vector4, Multiplication)
{
    const Vector4 a{ 1, 2, 3, 123 };
    const Vector4 b{ 1, 2, 3, 0 };
    const Vector4 c{ 1, 4, 9, 0 };

    EXPECT_TRUE(Math::EqualEstimate(a * b, c));

    EXPECT_TRUE(Math::EqualEstimate(a * 2.0f, Vector4{ 2, 4, 6, 246 }));
}

TEST(Vector4, Division)
{
    const Vector4 a{ 1, 0, 7, 1 };
    const Vector4 b{ 1, 2, 2, 1 };
    const Vector4 c{ 1, 0, 3.5f, 1 };

    EXPECT_TRUE(Math::EqualEstimate(a / b, c));

    EXPECT_TRUE(Math::EqualEstimate(b / 2.0f, Vector4{ 0.5f, 1.0f, 1.0f, 0.5f }));
}

TEST(Vector4, Dot)
{
    std::mt19937 mt(0);
    std::uniform_real_distribution<float> dist(-5, 5);

    const auto dotRef = [](const Vector4 lhs, const Vector4 rhs) {
        // non-SIMD version for testing
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
    };

    for (int32_t i = 0; i < 100'000; ++i)
    {
        const Vector4 a{ dist(mt), dist(mt), dist(mt), dist(mt) };
        const Vector4 b{ dist(mt), dist(mt), dist(mt), dist(mt) };

        const float actual = Math::Dot(a, b);
        const float expected = dotRef(a, b);
        const bool result = Math::EqualEstimate(actual, expected, 1e-5f);
        ASSERT_TRUE(result);
    }
}

TEST(Vector4, AbsNeg)
{
    const Vector4 a{ -1.0f, 2.0f, -0.0f, 0.0f };
    EXPECT_EQ(Math::Abs(a), Vector4(1.0f, 2.0f, 0.0f, 0.0f));
    EXPECT_TRUE(Math::EqualEstimate(-a, Vector4{ 1.0f, -2.0f, 0.0f, 0.0f }));
}

TEST(Vector4, MinMax)
{
    const Vector4 a{ 1, 5, 10, 1 };
    const Vector4 b{ 10, 2, 3, 1 };
    EXPECT_EQ(Math::Max(a, b), Vector4(10, 5, 10, 1));
    EXPECT_EQ(Math::Min(a, b), Vector4(1, 2, 3, 1));
}

TEST(Vector4, Clamp)
{
    const Vector4 greater{ 10.5f };
    const Vector4 fits{ 5.5f };
    const Vector4 less{ -7.5f };

    const Vector4 min{ -1.0f };
    const Vector4 max{ 7.0f };

    EXPECT_EQ(Math::Clamp(greater, min, max), max);
    EXPECT_EQ(Math::Clamp(fits, min, max), fits);
    EXPECT_EQ(Math::Clamp(less, min, max), min);
}

TEST(Vector4, Length)
{
    std::mt19937 mt(0xdeadbeef);
    std::uniform_real_distribution<float> dist(-5, 5);

    const auto lenRef = [](const Vector4 lhs) {
        // non-SIMD version for testing
        return lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z + lhs.w * lhs.w;
    };

    for (int32_t i = 0; i < 100'000; ++i)
    {
        const Vector4 a{ dist(mt), dist(mt), dist(mt), dist(mt) };

        constexpr float kTolerance = 1e-3f;
        ASSERT_TRUE(Math::EqualEstimate(Math::LengthSquared(a), lenRef(a), kTolerance));
        ASSERT_TRUE(Math::EqualEstimate(Math::Length(a), std::sqrt(lenRef(a)), kTolerance));
    }
}

TEST(Vector4, Normalize)
{
    const Vector4 a{ 1.0f, 0.0f, 0.0f, 0.0f };
    EXPECT_TRUE(Math::EqualEstimate(Math::Normalize(a), a));
    EXPECT_TRUE(Math::EqualEstimate(Math::NormalizeEstimate(a), a, 1e-3f));

    const Vector4 b{ 1.0f, 2.0f, 3.0f, -1.0f };
    const float length = 3.872983346f;
    const Vector4 expected{ 1.0f / length, 2.0f / length, 3.0f / length, -1.0f / length };
    const Vector4 actual = Math::Normalize(b);
    EXPECT_TRUE(Math::EqualEstimate(actual, expected, 1e-5f));
}
