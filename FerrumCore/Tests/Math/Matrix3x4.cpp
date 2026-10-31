#include <Core/Math/Matrix3x4.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    Matrix4x4 CreateAffine(const Vector3 translation, const float angle, const Vector3 scale)
    {
        return Matrix4x4::Scale(scale) * Matrix4x4::RotationY(angle) * Matrix4x4::Translation(translation);
    }
} // namespace


TEST(Matrix3x4, RoundTrip)
{
    static_assert(sizeof(Matrix3x4) == 48);

    const Matrix4x4 source = CreateAffine(Vector3(1, 2, 3), 0.7f, Vector3(2, 1, 0.5f));
    const Matrix3x4 affine = Matrix3x4::FromMatrix4x4(source);
    EXPECT_TRUE(Math::CmpEqual(Matrix3x4::ToMatrix4x4(affine), source));
    EXPECT_TRUE(Math::CmpEqual(affine.GetTranslation(), Vector3(1, 2, 3)));

    EXPECT_TRUE(Math::CmpEqual(Matrix3x4::ToMatrix4x4(Matrix3x4::kIdentity), Matrix4x4::kIdentity));
}


TEST(Matrix3x4, TransformPoint)
{
    const Matrix4x4 source = CreateAffine(Vector3(-4, 5, 6), 1.3f, Vector3(3, 2, 1));
    const Matrix3x4 affine = Matrix3x4::FromMatrix4x4(source);

    const Vector3 point(0.5f, -2, 7);
    const Vector4 expected = Vector4(point, 1.0f) * source;
    EXPECT_TRUE(Math::CmpEqual(Math::TransformPoint(affine, point), Vector3(expected.x, expected.y, expected.z), 1e-4f));
}


TEST(Matrix3x4, ComposeMatchesMatrix4x4)
{
    const Matrix4x4 local = CreateAffine(Vector3(1, 0, -2), 0.4f, Vector3(1, 2, 3));
    const Matrix4x4 parent = CreateAffine(Vector3(10, 20, 30), -1.1f, Vector3(0.5f, 0.5f, 0.5f));

    const Matrix3x4 composed = Matrix3x4::FromMatrix4x4(local) * Matrix3x4::FromMatrix4x4(parent);
    EXPECT_TRUE(Math::CmpEqual(Matrix3x4::ToMatrix4x4(composed), local * parent, 1e-4f));
}
