#include <FeCore/Math/Matrix4x4F.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(Matrix4x4, Create)
{
    Matrix4x4F m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.m_rows[0], Vector4F(+1.0f, +2.0f, +3.0f, +4.0f));
    EXPECT_EQ(m.m_rows[1], Vector4F(+5.0f, +6.0f, +7.0f, +8.0f));
    EXPECT_EQ(m.m_rows[2], Vector4F(-1.0f, -2.0f, -3.0f, -4.0f));
    EXPECT_EQ(m.m_rows[3], Vector4F(-5.0f, -6.0f, -7.0f, -8.0f));

    m = Matrix4x4F::FromColumns({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.m_rows[0], Vector4F(+1.0f, +5.0f, -1.0f, -5.0f));
    EXPECT_EQ(m.m_rows[1], Vector4F(+2.0f, +6.0f, -2.0f, -6.0f));
    EXPECT_EQ(m.m_rows[2], Vector4F(+3.0f, +7.0f, -3.0f, -7.0f));
    EXPECT_EQ(m.m_rows[3], Vector4F(+4.0f, +8.0f, -4.0f, -8.0f));

    m = Matrix4x4F::Identity();
    EXPECT_EQ(m.m_rows[0], Vector4F::AxisX());
    EXPECT_EQ(m.m_rows[1], Vector4F::AxisY());
    EXPECT_EQ(m.m_rows[2], Vector4F::AxisZ());
    EXPECT_EQ(m.m_rows[3], Vector4F::AxisW());

    m = Matrix4x4F::Zero();
    EXPECT_EQ(m.m_rows[0], Vector4F::Zero());
    EXPECT_EQ(m.m_rows[1], Vector4F::Zero());
    EXPECT_EQ(m.m_rows[2], Vector4F::Zero());
    EXPECT_EQ(m.m_rows[3], Vector4F::Zero());

    m = Matrix4x4F::Scale(Vector3F(1, 2, 3));
    EXPECT_EQ(m.m_rows[0], Vector4F(1.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_EQ(m.m_rows[1], Vector4F(0.0f, 2.0f, 0.0f, 0.0f));
    EXPECT_EQ(m.m_rows[2], Vector4F(0.0f, 0.0f, 3.0f, 0.0f));
    EXPECT_EQ(m.m_rows[3], Vector4F(0.0f, 0.0f, 0.0f, 1.0f));

    m = Matrix4x4F::Translation(Vector3F(1, 2, 3));
    EXPECT_EQ(m.m_rows[0], Vector4F(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_EQ(m.m_rows[1], Vector4F(0.0f, 1.0f, 0.0f, 2.0f));
    EXPECT_EQ(m.m_rows[2], Vector4F(0.0f, 0.0f, 1.0f, 3.0f));
    EXPECT_EQ(m.m_rows[3], Vector4F(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Matrix4x4, Rotation)
{
    using namespace Math;

    const float angle1 = 60.0f / 360.0f * 2 * Constants::PI;
    const float angle2 = 30.0f / 360.0f * 2 * Constants::PI;

    Matrix4x4F m = Matrix4x4F::RotationX(angle1);
    EXPECT_TRUE(EqualEstimate(m.m_rows[0], Vector4F(+1.0000000f, +0.0000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[1], Vector4F(+0.0000000f, +0.5000000f, -0.8660254f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[2], Vector4F(+0.0000000f, +0.8660254f, +0.5000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[3], Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));

    m = Matrix4x4F::RotationY(angle1);
    EXPECT_TRUE(EqualEstimate(m.m_rows[0], Vector4F(+0.5000000f, +0.0000000f, +0.8660254f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[1], Vector4F(+0.0000000f, +1.0000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[2], Vector4F(-0.8660254f, +0.0000000f, +0.5000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[3], Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));

    m = Matrix4x4F::RotationZ(angle1);
    EXPECT_TRUE(EqualEstimate(m.m_rows[0], Vector4F(+0.5000000f, -0.8660254f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[1], Vector4F(+0.8660254f, +0.5000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[2], Vector4F(-0.0000000f, +0.0000000f, +1.0000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[3], Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));

    m = Matrix4x4F::RotationX(angle2);
    EXPECT_TRUE(EqualEstimate(m.m_rows[0], Vector4F(+1.0000000f, +0.0000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[1], Vector4F(+0.0000000f, +0.8660254f, -0.5000000f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[2], Vector4F(+0.0000000f, +0.5000000f, +0.8660254f, +0.0000000f)));
    EXPECT_TRUE(EqualEstimate(m.m_rows[3], Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));
}

TEST(Matrix4x4, DataAccess)
{
    const Matrix4x4F m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    constexpr float data[]{ 1, 2, 3, 4, 5, 6, 7, 8, -1, -2, -3, -4, -5, -6, -7, -8 };
    for (size_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(data[i], m.RowMajorData()[i]);
    }
}

TEST(Matrix4x4, RowAccess)
{
    Matrix4x4F m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.m_rows[0], Vector4F(1, 2, 3, 4));
    m.m_rows[0] = Vector4F(9, 8, 7, 6);
    EXPECT_EQ(m.m_rows[0], Vector4F(9, 8, 7, 6));
    m.m_rows[1] = Vector4F(1, 2, 3, 4);
    EXPECT_EQ(m.m_rows[1], Vector4F(1, 2, 3, 4));
    m.m_rows[3] = Vector4F(9, 8, 7, 6);
    EXPECT_EQ(m.m_rows[3], Vector4F(9, 8, 7, 6));
    m.m_rows[3] = Vector4F(1, 2, 3, 4);
    EXPECT_EQ(m.m_rows[3], Vector4F(1, 2, 3, 4));

    m = Matrix4x4F::Identity();
    EXPECT_EQ(m.m_rows[0], Vector4F::AxisX());
    EXPECT_EQ(m.m_rows[1], Vector4F::AxisY());
    EXPECT_EQ(m.m_rows[2], Vector4F::AxisZ());
}

TEST(Matrix4x4, ColumnAccess)
{
    Matrix4x4F m = Matrix4x4F::FromColumns({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });

    EXPECT_EQ(Math::ExtractColumn<0>(m), Vector4F(1, 2, 3, 4));

    Math::ReplaceColumn<0>(m, { 9, 8, 7, 6 });
    EXPECT_EQ(Math::ExtractColumn<0>(m), Vector4F(9, 8, 7, 6));

    Math::ReplaceColumn<1>(m, { 1, 2, 3, 4 });
    EXPECT_EQ(Math::ExtractColumn<1>(m), Vector4F(1, 2, 3, 4));

    Math::ReplaceColumn<3>(m, { 9, 8, 7, 6 });
    EXPECT_EQ(Math::ExtractColumn<3>(m), Vector4F(9, 8, 7, 6));

    Math::ReplaceColumn<3>(m, { 1, 2, 3, 4 });
    EXPECT_EQ(Math::ExtractColumn<3>(m), Vector4F(1, 2, 3, 4));
}

TEST(Matrix4x4, ElementAccess)
{
    Matrix4x4F m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.m_00, 1);
    EXPECT_EQ(m.m_33, -m.m_13);
    m.m_00 = 123;
    EXPECT_EQ(m.m_00, 123);
}

TEST(Matrix4x4, Add)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(-9.641f, -8.484f, -5.599f, +8.573f);
    a.m_rows[1] = Vector4F(-6.406f, -2.276f, +5.028f, -8.149f);
    a.m_rows[2] = Vector4F(-2.633f, +1.400f, -4.628f, -0.243f);
    a.m_rows[3] = Vector4F(-3.858f, -3.307f, -4.033f, -9.087f);

    Matrix4x4F b{};
    b.m_rows[0] = Vector4F(-3.354f, +4.542f, -4.386f, +1.882f);
    b.m_rows[1] = Vector4F(-0.331f, +8.668f, +0.408f, +7.669f);
    b.m_rows[2] = Vector4F(+7.232f, -2.686f, +2.335f, +4.554f);
    b.m_rows[3] = Vector4F(+4.270f, +3.241f, +4.594f, -9.493f);

    const Matrix4x4F m = a + b;
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[0], Vector4F(-12.995f, -3.942f, -9.985f, +10.455f)));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[1], Vector4F(-06.737f, +6.392f, +5.436f, -00.480f)));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[2], Vector4F(+04.599f, -1.286f, -2.293f, +04.311f)));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[3], Vector4F(+00.412f, -0.066f, +0.561f, -18.580f)));

    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_TRUE(Math::EqualEstimate(m.m_values[i], a.m_values[i] + b.m_values[i]));
}

TEST(Matrix4x4, Subtract)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(-9.641f, -8.484f, -5.599f, +8.573f);
    a.m_rows[1] = Vector4F(-6.406f, -2.276f, +5.028f, -8.149f);
    a.m_rows[2] = Vector4F(-2.633f, +1.400f, -4.628f, -0.243f);
    a.m_rows[3] = Vector4F(-3.858f, -3.307f, -4.033f, -9.087f);

    Matrix4x4F b{};
    b.m_rows[0] = Vector4F(-3.354f, +4.542f, -4.386f, +1.882f);
    b.m_rows[1] = Vector4F(-0.331f, +8.668f, +0.408f, +7.669f);
    b.m_rows[2] = Vector4F(+7.232f, -2.686f, +2.335f, +4.554f);
    b.m_rows[3] = Vector4F(+4.270f, +3.241f, +4.594f, -9.493f);

    const Matrix4x4F m = a - b;
    EXPECT_TRUE(Math::EqualEstimate(m, a + -b));

    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_TRUE(Math::EqualEstimate(m.m_values[i], a.m_values[i] - b.m_values[i]));
}

TEST(Matrix4x4, MultiplyByScalar)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(-9.641f, -8.484f, -5.599f, +8.573f);
    a.m_rows[1] = Vector4F(-6.406f, -2.276f, +5.028f, -8.149f);
    a.m_rows[2] = Vector4F(-2.633f, +1.400f, -4.628f, -0.243f);
    a.m_rows[3] = Vector4F(-3.858f, -3.307f, -4.033f, -9.087f);

    const Matrix4x4F m = a * 2.3f;
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_TRUE(Math::EqualEstimate(m.m_values[i], a.m_values[i] * 2.3f));
}

TEST(Matrix4x4, DivideByScalar)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(-9.641f, -8.484f, -5.599f, +8.573f);
    a.m_rows[1] = Vector4F(-6.406f, -2.276f, +5.028f, -8.149f);
    a.m_rows[2] = Vector4F(-2.633f, +1.400f, -4.628f, -0.243f);
    a.m_rows[3] = Vector4F(-3.858f, -3.307f, -4.033f, -9.087f);

    const Matrix4x4F m = a / 2.3f;
    for (uint32_t i = 0; i < 16; ++i)
        EXPECT_TRUE(Math::EqualEstimate(m.m_values[i], a.m_values[i] / 2.3f));
}

TEST(Matrix4x4, Multiply)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(-9.641f, -8.484f, -5.599f, +8.573f);
    a.m_rows[1] = Vector4F(-6.406f, -2.276f, +5.028f, -8.149f);
    a.m_rows[2] = Vector4F(-2.633f, +1.400f, -4.628f, -0.243f);
    a.m_rows[3] = Vector4F(-3.858f, -3.307f, -4.033f, -9.087f);

    Matrix4x4F b{};
    b.m_rows[0] = Vector4F(-3.354f, +4.542f, -4.386f, +1.882f);
    b.m_rows[1] = Vector4F(-0.331f, +8.668f, +0.408f, +7.669f);
    b.m_rows[2] = Vector4F(+7.232f, -2.686f, +2.335f, +4.554f);
    b.m_rows[3] = Vector4F(+4.270f, +3.241f, +4.594f, -9.493f);

    const Matrix4x4F m = a * b;
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[0], Vector4F(+31.258860f, -74.504727f, +65.134651f, -190.089493f), 1e-5f));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[1], Vector4F(+23.805346f, -88.740537f, +01.471982f, +070.745233f), 1e-5f));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[2], Vector4F(-26.139624f, +11.819359f, +00.196816f, -012.987819f), 1e-5f));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[3], Vector4F(-53.933797f, -64.806441f, -35.590801f, +035.274470f), 1e-5f));
}

TEST(Matrix4x4, TransformVector)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(5, 4, 9, 7);
    a.m_rows[1] = Vector4F(6, 7, 8, 7);
    a.m_rows[2] = Vector4F(3, 9, 5, 3);
    a.m_rows[3] = Vector4F(6, 6, 9, 8);

    const Vector4F b{ 4, 6, 7, 8 };
    const Vector4F r = b * a;

    EXPECT_TRUE(Math::EqualEstimate(r, Vector4F(125, 169, 191, 155), 1e-5f));
}

TEST(Matrix4x4, Transpose)
{
    Matrix4x4F a{};
    a.m_rows[0] = Vector4F(+1, +2, +3, +4);
    a.m_rows[1] = Vector4F(+5, +6, +7, +8);
    a.m_rows[2] = Vector4F(-1, -2, -3, -4);
    a.m_rows[3] = Vector4F(-5, -6, -7, -8);

    const Matrix4x4F m = Math::Transpose(a);

    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[0], Vector4F(1, 5, -1, -5)));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[1], Vector4F(2, 6, -2, -6)));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[2], Vector4F(3, 7, -3, -7)));
    EXPECT_TRUE(Math::EqualEstimate(m.m_rows[3], Vector4F(4, 8, -4, -8)));
}

TEST(Matrix4x4, InverseTransform)
{
    Matrix4x4F m = Matrix4x4F::Translation(Vector3F(1, 2, 3));
    m = m * Matrix4x4F::RotationX(Math::Constants::PI / 4.0f);
    m = m * Matrix4x4F::Scale(Vector3F(4, 5, 6));

    const Matrix4x4F identity = m * Math::InverseTransform(m);
    EXPECT_TRUE(Math::EqualEstimate(identity, Matrix4x4F::Identity()));
}
