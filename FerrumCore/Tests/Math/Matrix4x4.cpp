#include <FeCore/Math/Matrix4x4F.h>
#include <gtest/gtest.h>

using FE::Matrix4x4F;
using FE::Vector3F;
using FE::Vector4F;

TEST(Matrix4x4, Create)
{
    auto m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.GetRow(0), Vector4F(+1.0f, +2.0f, +3.0f, +4.0f));
    EXPECT_EQ(m.GetRow(1), Vector4F(+5.0f, +6.0f, +7.0f, +8.0f));
    EXPECT_EQ(m.GetRow(2), Vector4F(-1.0f, -2.0f, -3.0f, -4.0f));
    EXPECT_EQ(m.GetRow(3), Vector4F(-5.0f, -6.0f, -7.0f, -8.0f));

    m = Matrix4x4F::FromColumns({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.GetRow(0), Vector4F(+1.0f, +5.0f, -1.0f, -5.0f));
    EXPECT_EQ(m.GetRow(1), Vector4F(+2.0f, +6.0f, -2.0f, -6.0f));
    EXPECT_EQ(m.GetRow(2), Vector4F(+3.0f, +7.0f, -3.0f, -7.0f));
    EXPECT_EQ(m.GetRow(3), Vector4F(+4.0f, +8.0f, -4.0f, -8.0f));

    m = Matrix4x4F::GetIdentity();
    EXPECT_EQ(m.GetRow(0), Vector4F::GetUnitX());
    EXPECT_EQ(m.GetRow(1), Vector4F::GetUnitY());
    EXPECT_EQ(m.GetRow(2), Vector4F::GetUnitZ());
    EXPECT_EQ(m.GetRow(3), Vector4F::GetUnitW());

    m = Matrix4x4F::GetZero();
    EXPECT_EQ(m.GetRow(0), Vector4F::GetZero());
    EXPECT_EQ(m.GetRow(1), Vector4F::GetZero());
    EXPECT_EQ(m.GetRow(2), Vector4F::GetZero());
    EXPECT_EQ(m.GetRow(3), Vector4F::GetZero());

    m = Matrix4x4F::CreateScale(Vector3F(1, 2, 3));
    EXPECT_EQ(m.GetRow(0), Vector4F(1.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_EQ(m.GetRow(1), Vector4F(0.0f, 2.0f, 0.0f, 0.0f));
    EXPECT_EQ(m.GetRow(2), Vector4F(0.0f, 0.0f, 3.0f, 0.0f));
    EXPECT_EQ(m.GetRow(3), Vector4F(0.0f, 0.0f, 0.0f, 1.0f));

    m = Matrix4x4F::CreateDiagonal(Vector4F(1, 2, 3, 4));
    EXPECT_EQ(m.GetRow(0), Vector4F(1.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_EQ(m.GetRow(1), Vector4F(0.0f, 2.0f, 0.0f, 0.0f));
    EXPECT_EQ(m.GetRow(2), Vector4F(0.0f, 0.0f, 3.0f, 0.0f));
    EXPECT_EQ(m.GetRow(3), Vector4F(0.0f, 0.0f, 0.0f, 4.0f));
    EXPECT_EQ(m, Matrix4x4F::CreateDiagonal(1, 2, 3, 4));

    m = Matrix4x4F::CreateTranslation(Vector3F(1, 2, 3));
    EXPECT_EQ(m.GetRow(0), Vector4F(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_EQ(m.GetRow(1), Vector4F(0.0f, 1.0f, 0.0f, 2.0f));
    EXPECT_EQ(m.GetRow(2), Vector4F(0.0f, 0.0f, 1.0f, 3.0f));
    EXPECT_EQ(m.GetRow(3), Vector4F(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Matrix4x4, Rotation)
{
    using namespace FE::Math;

    auto m = Matrix4x4F::CreateRotationX(ToRadians(60.f));
    EXPECT_TRUE(m.GetRow(0).IsApproxEqualTo(Vector4F(+1.0000000f, +0.0000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(1).IsApproxEqualTo(Vector4F(+0.0000000f, +0.5000000f, -0.8660254f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(2).IsApproxEqualTo(Vector4F(+0.0000000f, +0.8660254f, +0.5000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(3).IsApproxEqualTo(Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));

    m = Matrix4x4F::CreateRotationY(ToRadians(60.f));
    EXPECT_TRUE(m.GetRow(0).IsApproxEqualTo(Vector4F(+0.5000000f, +0.0000000f, +0.8660254f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(1).IsApproxEqualTo(Vector4F(+0.0000000f, +1.0000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(2).IsApproxEqualTo(Vector4F(-0.8660254f, +0.0000000f, +0.5000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(3).IsApproxEqualTo(Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));

    m = Matrix4x4F::CreateRotationZ(ToRadians(60.f));
    EXPECT_TRUE(m.GetRow(0).IsApproxEqualTo(Vector4F(+0.5000000f, -0.8660254f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(1).IsApproxEqualTo(Vector4F(+0.8660254f, +0.5000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(2).IsApproxEqualTo(Vector4F(-0.0000000f, +0.0000000f, +1.0000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(3).IsApproxEqualTo(Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));

    m = Matrix4x4F::CreateRotationX(ToRadians(30.0f));
    EXPECT_TRUE(m.GetRow(0).IsApproxEqualTo(Vector4F(+1.0000000f, +0.0000000f, +0.0000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(1).IsApproxEqualTo(Vector4F(+0.0000000f, +0.8660254f, -0.5000000f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(2).IsApproxEqualTo(Vector4F(+0.0000000f, +0.5000000f, +0.8660254f, +0.0000000f)));
    EXPECT_TRUE(m.GetRow(3).IsApproxEqualTo(Vector4F(+0.0000000f, +0.0000000f, +0.0000000f, +1.0000000f)));
}

TEST(Matrix4x4, DataAccess)
{
    auto m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    std::array<float, 16> data{ 1, 2, 3, 4, 5, 6, 7, 8, -1, -2, -3, -4, -5, -6, -7, -8 };
    for (size_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(data[i], m.RowMajorData()[i]);
    }
}

TEST(Matrix4x4, RowAccess)
{
    auto m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.GetRow(0), Vector4F(1, 2, 3, 4));
    m.SetRow(0, 9, 8, 7, 6);
    EXPECT_EQ(m.GetRow(0), Vector4F(9, 8, 7, 6));
    m.SetRow(1, Vector4F(1, 2, 3, 4));
    EXPECT_EQ(m.GetRow(1), Vector4F(1, 2, 3, 4));
    m.SetRow(3, Vector3F(9, 8, 7), 6);
    EXPECT_EQ(m.GetRow(3), Vector4F(9, 8, 7, 6));
    m.SetRow(3, Vector4F(1, 2, 3, 4));
    EXPECT_EQ(m.GetRow(3), Vector4F(1, 2, 3, 4));

    m = Matrix4x4F::GetIdentity();
    EXPECT_EQ(m.GetBasisX(), Vector4F::GetUnitX());
    EXPECT_EQ(m.GetBasisY(), Vector4F::GetUnitY());
    EXPECT_EQ(m.GetBasisZ(), Vector4F::GetUnitZ());
}

TEST(Matrix4x4, ColumnAccess)
{
    auto m = Matrix4x4F::FromColumns({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m.GetColumn(0), Vector4F(1, 2, 3, 4));
    m.SetColumn(0, 9, 8, 7, 6);
    EXPECT_EQ(m.GetColumn(0), Vector4F(9, 8, 7, 6));
    m.SetColumn(1, Vector4F(1, 2, 3, 4));
    EXPECT_EQ(m.GetColumn(1), Vector4F(1, 2, 3, 4));
    m.SetColumn(3, Vector3F(9, 8, 7), 6);
    EXPECT_EQ(m.GetColumn(3), Vector4F(9, 8, 7, 6));
    m.SetColumn(3, Vector4F(1, 2, 3, 4));
    EXPECT_EQ(m.GetColumn(3), Vector4F(1, 2, 3, 4));
}

TEST(Matrix4x4, ElementAccess)
{
    auto m = Matrix4x4F::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 });
    EXPECT_EQ(m(0, 0), 1);
    EXPECT_EQ(m(3, 3), -m(1, 3));
    m(0, 0) = 123;
    EXPECT_EQ(m(0, 0), 123);
}

TEST(Matrix4x4, Add)
{
    Matrix4x4F a{};
    a.SetRow(0, -9.641f, -8.484f, -5.599f, +8.573f);
    a.SetRow(1, -6.406f, -2.276f, +5.028f, -8.149f);
    a.SetRow(2, -2.633f, +1.400f, -4.628f, -0.243f);
    a.SetRow(3, -3.858f, -3.307f, -4.033f, -9.087f);

    Matrix4x4F b{};
    b.SetRow(0, -3.354f, +4.542f, -4.386f, +1.882f);
    b.SetRow(1, -0.331f, +8.668f, +0.408f, +7.669f);
    b.SetRow(2, +7.232f, -2.686f, +2.335f, +4.554f);
    b.SetRow(3, +4.270f, +3.241f, +4.594f, -9.493f);

    auto m = a + b;
    EXPECT_TRUE(m.GetRow(0).IsApproxEqualTo(Vector4F(-12.995000f, -3.942000f, -9.985000f, +10.455000f)));
    EXPECT_TRUE(m.GetRow(1).IsApproxEqualTo(Vector4F(-06.737000f, +6.392000f, +5.436000f, -00.480000f)));
    EXPECT_TRUE(m.GetRow(2).IsApproxEqualTo(Vector4F(+04.599000f, -1.286000f, -2.293000f, +04.311000f)));
    EXPECT_TRUE(m.GetRow(3).IsApproxEqualTo(Vector4F(+00.412000f, -0.066000f, +0.561000f, -18.580000f)));

    a += b;
    EXPECT_TRUE(a.IsApproxEqualTo(m));
}

TEST(Matrix4x4, Subtract)
{
    Matrix4x4F a{};
    a.SetRow(0, -9.641f, -8.484f, -5.599f, +8.573f);
    a.SetRow(1, -6.406f, -2.276f, +5.028f, -8.149f);
    a.SetRow(2, -2.633f, +1.400f, -4.628f, -0.243f);
    a.SetRow(3, -3.858f, -3.307f, -4.033f, -9.087f);

    Matrix4x4F b{};
    b.SetRow(0, -3.354f, +4.542f, -4.386f, +1.882f);
    b.SetRow(1, -0.331f, +8.668f, +0.408f, +7.669f);
    b.SetRow(2, +7.232f, -2.686f, +2.335f, +4.554f);
    b.SetRow(3, +4.270f, +3.241f, +4.594f, -9.493f);

    auto m = a - b;
    EXPECT_TRUE(m.IsApproxEqualTo(a + (-b)));

    a -= b;
    EXPECT_TRUE(a.IsApproxEqualTo(m));
}

TEST(Matrix4x4, MultiplyByScalar)
{
    EXPECT_TRUE((Matrix4x4F::GetIdentity() * 5).IsApproxEqualTo(Matrix4x4F::CreateDiagonal(5, 5, 5, 5)));
    Matrix4x4F m = Matrix4x4F::GetIdentity();
    m *= 5;
    EXPECT_TRUE(m.IsApproxEqualTo(Matrix4x4F::CreateDiagonal(5, 5, 5, 5)));
}

TEST(Matrix4x4, DivideByScalar)
{
    EXPECT_TRUE(Matrix4x4F::GetIdentity().IsApproxEqualTo(Matrix4x4F::CreateDiagonal(5, 5, 5, 5) / 5));
    Matrix4x4F m = Matrix4x4F::CreateDiagonal(5, 5, 5, 5);
    m /= 5;
    EXPECT_TRUE(m.IsApproxEqualTo(Matrix4x4F::GetIdentity()));
}

TEST(Matrix4x4, Multiply)
{
    Matrix4x4F a{};
    a.SetRow(0, -9.641f, -8.484f, -5.599f, +8.573f);
    a.SetRow(1, -6.406f, -2.276f, +5.028f, -8.149f);
    a.SetRow(2, -2.633f, +1.400f, -4.628f, -0.243f);
    a.SetRow(3, -3.858f, -3.307f, -4.033f, -9.087f);

    Matrix4x4F b{};
    b.SetRow(0, -3.354f, +4.542f, -4.386f, +1.882f);
    b.SetRow(1, -0.331f, +8.668f, +0.408f, +7.669f);
    b.SetRow(2, +7.232f, -2.686f, +2.335f, +4.554f);
    b.SetRow(3, +4.270f, +3.241f, +4.594f, -9.493f);

    auto m = a * b;
    EXPECT_TRUE(m.GetRow(0).IsApproxEqualTo(Vector4F(+31.258860f, -74.504727f, +65.134651f, -190.089493f)));
    EXPECT_TRUE(m.GetRow(1).IsApproxEqualTo(Vector4F(+23.805346f, -88.740537f, +01.471982f, +070.745233f)));
    EXPECT_TRUE(m.GetRow(2).IsApproxEqualTo(Vector4F(-26.139624f, +11.819359f, +00.196816f, -012.987819f)));
    EXPECT_TRUE(m.GetRow(3).IsApproxEqualTo(Vector4F(-53.933797f, -64.806441f, -35.590801f, +035.274470f)));

    a *= b;
    EXPECT_TRUE(a.IsApproxEqualTo(m));
}

TEST(Matrix4x4, MultiplyByVector)
{
    Matrix4x4F a{};
    a.SetRow(0, -3.354f, +4.542f, -4.386f, +1.882f);
    a.SetRow(1, -0.331f, +8.668f, +0.408f, +7.669f);
    a.SetRow(2, +7.232f, -2.686f, +2.335f, +4.554f);
    a.SetRow(3, +4.270f, +3.241f, +4.594f, -9.493f);

    Vector4F b{ -1.575, 6.826, -1.355, -4.982 };
    auto r = a * b;

    EXPECT_TRUE(r.IsApproxEqualTo(Vector4F(32.853148f, 20.929295f, -55.576989f, 56.467072f)));
}

TEST(Matrix4x4, Transpose)
{
    Matrix4x4F a{};
    a.SetRow(0, +1, +2, +3, +4);
    a.SetRow(1, +5, +6, +7, +8);
    a.SetRow(2, -1, -2, -3, -4);
    a.SetRow(3, -5, -6, -7, -8);

    EXPECT_TRUE(a.Transposed().GetRow(0).IsApproxEqualTo(Vector4F(1, 5, -1, -5)));
    EXPECT_TRUE(a.Transposed().GetRow(1).IsApproxEqualTo(Vector4F(2, 6, -2, -6)));
    EXPECT_TRUE(a.Transposed().GetRow(2).IsApproxEqualTo(Vector4F(3, 7, -3, -7)));
    EXPECT_TRUE(a.Transposed().GetRow(3).IsApproxEqualTo(Vector4F(4, 8, -4, -8)));
}

TEST(Matrix4x4, Determinant)
{
    Matrix4x4F a{};
    a.SetRow(0, -3.354f, +4.542f, -4.386f, +1.882f);
    a.SetRow(1, -0.331f, +8.668f, +0.408f, +7.669f);
    a.SetRow(2, +7.232f, -2.686f, +2.335f, +4.554f);
    a.SetRow(3, +4.270f, +3.241f, +4.594f, -9.493f);

    EXPECT_FLOAT_EQ(a.Determinant(), -3426.404247);
}

TEST(Matrix4x4, InverseTransform)
{
    auto m = Matrix4x4F::CreateTranslation(Vector3F(1, 2, 3));
    m *= Matrix4x4F::CreateRotationX(FE::Math::ToRadians(60.f));
    m *= Matrix4x4F::CreateScale(Vector3F(4, 5, 6));
    auto identity = m * m.InverseTransform();
    EXPECT_TRUE(identity.IsApproxEqualTo(Matrix4x4F::GetIdentity()));
}
