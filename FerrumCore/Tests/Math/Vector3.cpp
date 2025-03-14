#include <FeCore/Math/Random.h>
#include <FeCore/Math/Vector3.h>
#include <FeCore/Threading/Thread.h>
#include <gtest/gtest.h>
#include <random>

using namespace FE;

namespace
{
    DefaultRandom GRandom;

    FE_FORCE_INLINE float RandFloat()
    {
        return GRandom.RandFloat();
    }
} // namespace


TEST(Vector3, GetXYZ)
{
    const Vector3F vector{ 1, 2, 3 };
    EXPECT_EQ(vector.x, 1);
    EXPECT_EQ(vector.y, 2);
    EXPECT_EQ(vector.z, 3);

    EXPECT_EQ(vector.Data()[0], 1);
    EXPECT_EQ(vector.Data()[1], 2);
    EXPECT_EQ(vector.Data()[2], 3);
}

TEST(Vector3, CreateFunctions)
{
    EXPECT_EQ(Vector3F::Zero(), Vector3F(0));

    EXPECT_EQ(Vector3F::AxisX(), Vector3F(1, 0, 0));
    EXPECT_EQ(Vector3F::AxisY(), Vector3F(0, 1, 0));
    EXPECT_EQ(Vector3F::AxisZ(), Vector3F(0, 0, 1));
}

TEST(Vector3, Load)
{
    const float alignas(__m128) values[] = { 1, 2, 3, 4, 5 };
    const Vector3F vector1 = Vector3F::LoadAligned(values);
    EXPECT_EQ(vector1.x, 1);
    EXPECT_EQ(vector1.y, 2);
    EXPECT_EQ(vector1.z, 3);

    const Vector3F vector2 = Vector3F::LoadUnaligned(values + 1);
    EXPECT_EQ(vector2.x, 2);
    EXPECT_EQ(vector2.y, 3);
    EXPECT_EQ(vector2.z, 4);
}

TEST(Vector3, Equality)
{
    EXPECT_EQ(Vector3F(1, 2, 3), Vector3F(1, 2, 3));

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    EXPECT_EQ(Vector3F(_mm_setr_ps(1, 2, 3, RandFloat())), Vector3F(_mm_setr_ps(1, 2, 3, RandFloat())));

    EXPECT_NE(Vector3F(1, 2, 3), Vector3F(0, 2, 3));
    EXPECT_NE(Vector3F(1, 2, 3), Vector3F(1, 0, 3));
    EXPECT_NE(Vector3F(1, 2, 3), Vector3F(1, 2, 0));

    EXPECT_TRUE(Math::EqualEstimate(Vector3F(1, 2, 3), Vector3F(1.1f, 2.1f, 3.1f), 0.11f));
    EXPECT_TRUE(Math::EqualEstimate(Vector3F(0.01f), Vector3F::Zero(), 0.02f));
    EXPECT_TRUE(Math::EqualEstimate(Vector3F::Zero(), Vector3F(0.01f), 0.02f));
    EXPECT_TRUE(Math::EqualEstimate(Vector3F(0.0f), Vector3F(-0.0f)));

    EXPECT_FALSE(Math::EqualEstimate(Vector3F(1, 2, 3), Vector3F(1.2f, 2.1f, 3.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector3F(1, 2, 3), Vector3F(1.1f, 2.2f, 3.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector3F(1, 2, 3), Vector3F(1.1f, 2.1f, 3.2f), 0.11f));
}

TEST(Vector3, Comparison)
{
    Vector3F a{ 1, 2, 10 };
    Vector3F b{ 1, 5, 6 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();

    EXPECT_EQ(Math::CmpEqualMask(a, b), 0b001);
    EXPECT_EQ(Math::CmpLessMask(a, b), 0b010);
    EXPECT_EQ(Math::CmpGreaterMask(a, b), 0b100);
    EXPECT_EQ(Math::CmpLessEqualMask(a, b), 0b011);
    EXPECT_EQ(Math::CmpGreaterEqualMask(a, b), 0b101);
}

TEST(Vector3, Addition)
{
    Vector3F a{ 1, 2, 3 };
    Vector3F b{ 1, 2, 3 };
    Vector3F c{ 2, 4, 6 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a + b, c));
}

TEST(Vector3, Subtraction)
{
    Vector3F a{ 1, 2, 3 };
    Vector3F b{ 1, 2, 3 };
    Vector3F c{ 0, 0, 0 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a - b, c));
}

TEST(Vector3, Multiplication)
{
    Vector3F a{ 1, 2, 3 };
    Vector3F b{ 1, 2, 3 };
    Vector3F c{ 1, 4, 9 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a * b, c));

    EXPECT_TRUE(Math::EqualEstimate(a * 2.0f, Vector3F{ 2, 4, 6 }));
}

TEST(Vector3, Division)
{
    Vector3F a{ 1, 0, 7 };
    Vector3F b{ 1, 2, 2 };
    Vector3F c{ 1, 0, 3.5f };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a / b, c));

    EXPECT_TRUE(Math::EqualEstimate(b / 2.0f, Vector3F{ 0.5f, 1.0f, 1.0f }));
}

TEST(Vector3, Dot)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    const auto dotRef = [](PackedVector3F lhs, PackedVector3F rhs) {
        // non-SIMD version for testing
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    };

    for (int32_t i = 0; i < 100'000; ++i)
    {
        Vector3F a{ dist(mt), dist(mt), dist(mt) };
        Vector3F b{ dist(mt), dist(mt), dist(mt) };

        // Put in random garbage into the w component to ensure it's excluded from the calculations.
        a.m_values[3] = dist(mt);
        b.m_values[3] = dist(mt);

        ASSERT_TRUE(Math::EqualEstimate(Math::Dot(a, b), dotRef(PackedVector3F{ a }, PackedVector3F{ b })));
    }
}

TEST(Vector3, Cross)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    const auto crossRef = [](PackedVector3F lhs, PackedVector3F rhs) -> PackedVector3F {
        // non-SIMD version for testing
        const float x = lhs.y * rhs.z - rhs.y * lhs.z;
        const float y = lhs.z * rhs.x - rhs.z * lhs.x;
        const float z = lhs.x * rhs.y - rhs.x * lhs.y;
        return { x, y, z };
    };

    for (int32_t i = 0; i < 100'000; ++i)
    {
        Vector3F a{ dist(mt), dist(mt), dist(mt) };
        Vector3F b{ dist(mt), dist(mt), dist(mt) };

        // Put in random garbage into the w component to ensure it's excluded from the calculations.
        a.m_values[3] = dist(mt);
        b.m_values[3] = dist(mt);

        const Vector3F ref{ crossRef(PackedVector3F{ a }, PackedVector3F{ b }) };
        ASSERT_TRUE(Math::EqualEstimate(Math::Cross(a, b), ref));
    }
}

TEST(Vector3, AbsNeg)
{
    const Vector3F a{ -1.0f, 2.0f, -0.0f };
    EXPECT_EQ(Math::Abs(a), Vector3F(1.0f, 2.0f, 0.0f));
    EXPECT_TRUE(Math::EqualEstimate(-a, Vector3F{ 1.0f, -2.0f, 0.0f }));
}

TEST(Vector3, MinMax)
{
    const Vector3F a{ 1, 5, 10 };
    const Vector3F b{ 10, 2, 3 };
    EXPECT_EQ(Math::Max(a, b), Vector3F(10, 5, 10));
    EXPECT_EQ(Math::Min(a, b), Vector3F(1, 2, 3));
}

TEST(Vector3, Clamp)
{
    const Vector3F greater{ 10.5f };
    const Vector3F fits{ 5.5f };
    const Vector3F less{ -7.5f };

    const Vector3F min{ -1.0f };
    const Vector3F max{ 7.0f };

    EXPECT_EQ(Math::Clamp(greater, min, max), max);
    EXPECT_EQ(Math::Clamp(fits, min, max), fits);
    EXPECT_EQ(Math::Clamp(less, min, max), min);
}

TEST(Vector3, Length)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(-10000, 10000);

    const auto lenRef = [](PackedVector3F lhs) {
        // non-SIMD version for testing
        return lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z;
    };

    for (int32_t i = 0; i < 100'000; ++i)
    {
        Vector3F a{ dist(mt), dist(mt), dist(mt) };

        // Put in random garbage into the w component to ensure it's excluded from the calculations.
        a.m_values[3] = dist(mt);

        ASSERT_TRUE(Math::EqualEstimate(Math::LengthSquared(a), lenRef(PackedVector3F{ a })));
        ASSERT_TRUE(Math::EqualEstimate(Math::Length(a), std::sqrt(lenRef(PackedVector3F{ a }))));
    }
}

TEST(Vector3, Normalize)
{
    const Vector3F a{ 1.0f, 0.0f, 0.0f };
    EXPECT_TRUE(Math::EqualEstimate(Math::Normalize(a), a));
    EXPECT_TRUE(Math::EqualEstimate(Math::NormalizeEstimate(a), a, 1e-3f));

    const Vector3F b{ 1.0f, 2.0f, 3.0f };
    const float length = 3.7416573f;
    const Vector3F expected{ 1.0f / length, 2.0f / length, 3.0f / length };
    EXPECT_TRUE(Math::EqualEstimate(Math::Normalize(b), expected));
}
