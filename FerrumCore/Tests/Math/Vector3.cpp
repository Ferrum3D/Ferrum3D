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
    const Vector3 vector{ 1, 2, 3 };
    EXPECT_EQ(vector.x, 1);
    EXPECT_EQ(vector.y, 2);
    EXPECT_EQ(vector.z, 3);

    EXPECT_EQ(vector.Data()[0], 1);
    EXPECT_EQ(vector.Data()[1], 2);
    EXPECT_EQ(vector.Data()[2], 3);
}

TEST(Vector3, CreateFunctions)
{
    EXPECT_EQ(Vector3::Zero(), Vector3(0));

    EXPECT_EQ(Vector3::AxisX(), Vector3(1, 0, 0));
    EXPECT_EQ(Vector3::AxisY(), Vector3(0, 1, 0));
    EXPECT_EQ(Vector3::AxisZ(), Vector3(0, 0, 1));
}

TEST(Vector3, Load)
{
    const float alignas(__m128) values[] = { 1, 2, 3, 4, 5 };
    const Vector3 vector1 = Vector3::LoadAligned(values);
    EXPECT_EQ(vector1.x, 1);
    EXPECT_EQ(vector1.y, 2);
    EXPECT_EQ(vector1.z, 3);

    const Vector3 vector2 = Vector3::LoadUnaligned(values + 1);
    EXPECT_EQ(vector2.x, 2);
    EXPECT_EQ(vector2.y, 3);
    EXPECT_EQ(vector2.z, 4);
}

TEST(Vector3, Equality)
{
    EXPECT_EQ(Vector3(1, 2, 3), Vector3(1, 2, 3));

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    EXPECT_EQ(Vector3(_mm_setr_ps(1, 2, 3, RandFloat())), Vector3(_mm_setr_ps(1, 2, 3, RandFloat())));

    EXPECT_NE(Vector3(1, 2, 3), Vector3(0, 2, 3));
    EXPECT_NE(Vector3(1, 2, 3), Vector3(1, 0, 3));
    EXPECT_NE(Vector3(1, 2, 3), Vector3(1, 2, 0));

    EXPECT_TRUE(Math::EqualEstimate(Vector3(1, 2, 3), Vector3(1.1f, 2.1f, 3.1f), 0.11f));
    EXPECT_TRUE(Math::EqualEstimate(Vector3(0.01f), Vector3::Zero(), 0.02f));
    EXPECT_TRUE(Math::EqualEstimate(Vector3::Zero(), Vector3(0.01f), 0.02f));
    EXPECT_TRUE(Math::EqualEstimate(Vector3(0.0f), Vector3(-0.0f)));

    EXPECT_FALSE(Math::EqualEstimate(Vector3(1, 2, 3), Vector3(1.2f, 2.1f, 3.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector3(1, 2, 3), Vector3(1.1f, 2.2f, 3.1f), 0.11f));
    EXPECT_FALSE(Math::EqualEstimate(Vector3(1, 2, 3), Vector3(1.1f, 2.1f, 3.2f), 0.11f));
}

TEST(Vector3, Comparison)
{
    Vector3 a{ 1, 2, 10 };
    Vector3 b{ 1, 5, 6 };

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
    Vector3 a{ 1, 2, 3 };
    Vector3 b{ 1, 2, 3 };
    Vector3 c{ 2, 4, 6 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a + b, c));
}

TEST(Vector3, Subtraction)
{
    Vector3 a{ 1, 2, 3 };
    Vector3 b{ 1, 2, 3 };
    Vector3 c{ 0, 0, 0 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a - b, c));
}

TEST(Vector3, Multiplication)
{
    Vector3 a{ 1, 2, 3 };
    Vector3 b{ 1, 2, 3 };
    Vector3 c{ 1, 4, 9 };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a * b, c));

    EXPECT_TRUE(Math::EqualEstimate(a * 2.0f, Vector3{ 2, 4, 6 }));
}

TEST(Vector3, Division)
{
    Vector3 a{ 1, 0, 7 };
    Vector3 b{ 1, 2, 2 };
    Vector3 c{ 1, 0, 3.5f };

    // Put in random garbage into the w component to ensure it's excluded from the calculations.
    a.m_values[3] = RandFloat();
    b.m_values[3] = RandFloat();
    c.m_values[3] = RandFloat();

    EXPECT_TRUE(Math::EqualEstimate(a / b, c));

    EXPECT_TRUE(Math::EqualEstimate(b / 2.0f, Vector3{ 0.5f, 1.0f, 1.0f }));
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
        Vector3 a{ dist(mt), dist(mt), dist(mt) };
        Vector3 b{ dist(mt), dist(mt), dist(mt) };

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
        Vector3 a{ dist(mt), dist(mt), dist(mt) };
        Vector3 b{ dist(mt), dist(mt), dist(mt) };

        // Put in random garbage into the w component to ensure it's excluded from the calculations.
        a.m_values[3] = dist(mt);
        b.m_values[3] = dist(mt);

        const Vector3 ref{ crossRef(PackedVector3F{ a }, PackedVector3F{ b }) };
        ASSERT_TRUE(Math::EqualEstimate(Math::Cross(a, b), ref));
    }
}

TEST(Vector3, AbsNeg)
{
    const Vector3 a{ -1.0f, 2.0f, -0.0f };
    EXPECT_EQ(Math::Abs(a), Vector3(1.0f, 2.0f, 0.0f));
    EXPECT_TRUE(Math::EqualEstimate(-a, Vector3{ 1.0f, -2.0f, 0.0f }));
}

TEST(Vector3, MinMax)
{
    const Vector3 a{ 1, 5, 10 };
    const Vector3 b{ 10, 2, 3 };
    EXPECT_EQ(Math::Max(a, b), Vector3(10, 5, 10));
    EXPECT_EQ(Math::Min(a, b), Vector3(1, 2, 3));
}

TEST(Vector3, Clamp)
{
    const Vector3 greater{ 10.5f };
    const Vector3 fits{ 5.5f };
    const Vector3 less{ -7.5f };

    const Vector3 min{ -1.0f };
    const Vector3 max{ 7.0f };

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
        Vector3 a{ dist(mt), dist(mt), dist(mt) };

        // Put in random garbage into the w component to ensure it's excluded from the calculations.
        a.m_values[3] = dist(mt);

        ASSERT_TRUE(Math::EqualEstimate(Math::LengthSquared(a), lenRef(PackedVector3F{ a })));
        ASSERT_TRUE(Math::EqualEstimate(Math::Length(a), std::sqrt(lenRef(PackedVector3F{ a }))));
    }
}

TEST(Vector3, Normalize)
{
    const Vector3 a{ 1.0f, 0.0f, 0.0f };
    EXPECT_TRUE(Math::EqualEstimate(Math::Normalize(a), a));
    EXPECT_TRUE(Math::EqualEstimate(Math::NormalizeEstimate(a), a, 1e-3f));

    const Vector3 b{ 1.0f, 2.0f, 3.0f };
    const float length = 3.7416573f;
    const Vector3 expected{ 1.0f / length, 2.0f / length, 3.0f / length };
    EXPECT_TRUE(Math::EqualEstimate(Math::Normalize(b), expected));
}
