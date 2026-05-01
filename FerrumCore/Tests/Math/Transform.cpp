#include <FeCore/Math/Transform.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    Quaternion MakeTestRotation()
    {
        return Math::Normalize(Quaternion::RotationX(Constants::kPI / 6.0f) * Quaternion::RotationY(-Constants::kPI / 4.0f)
                               * Quaternion::RotationZ(Constants::kPI / 3.0f));
    }


    void ExpectSameRotation(const Quaternion actual, const Quaternion expected)
    {
        EXPECT_TRUE(Math::CmpEqual(Matrix4x4::Rotation(actual), Matrix4x4::Rotation(expected), 1e-5f));
    }
} // namespace


TEST(Transform, ToMatrix)
{
    const Vector3 translation{ 1.5f, -2.0f, 3.25f };
    const Quaternion rotation = MakeTestRotation();
    constexpr float scaleValue = 2.5f;

    const Transform transform = Transform::Create(translation, rotation, scaleValue);
    const Matrix4x4 matrix = Transform::ToMatrix(transform);

    Matrix4x4 expected = Matrix4x4::Rotation(rotation);
    const Vector4 scale{ scaleValue };
    expected.m_rows[0] *= scale;
    expected.m_rows[1] *= scale;
    expected.m_rows[2] *= scale;
    expected.m_rows[3] = Vector4{ translation, 1.0f };

    EXPECT_TRUE(Math::CmpEqual(matrix, expected, 1e-5f));
}


TEST(Transform, FromMatrix)
{
    const Vector3 translation{ -4.0f, 5.5f, 1.25f };
    const Quaternion rotation = MakeTestRotation();
    constexpr float scaleValue = 3.0f;

    const Matrix4x4 matrix =
        Matrix4x4::Scale(Vector3{ scaleValue }) * Matrix4x4::Rotation(rotation) * Matrix4x4::Translation(translation);

    const Transform transform = Transform::FromMatrix(matrix);

    EXPECT_TRUE(Math::CmpEqual(transform.Translation(), translation));
    EXPECT_TRUE(Math::CmpEqual(transform.Scale(), scaleValue));
    ExpectSameRotation(transform.Rotation(), rotation);
}


TEST(Transform, RoundTripMatrixTransformMatrix)
{
    const Vector3 translation{ 2.0f, -3.0f, 4.5f };
    const Quaternion rotation = MakeTestRotation();
    constexpr float scaleValue = 1.75f;

    const Matrix4x4 source =
        Matrix4x4::Scale(Vector3{ scaleValue }) * Matrix4x4::Rotation(rotation) * Matrix4x4::Translation(translation);

    const Transform transform = Transform::FromMatrix(source);
    const Matrix4x4 rebuilt = Transform::ToMatrix(transform);

    EXPECT_TRUE(Math::CmpEqual(rebuilt, source, 1e-5f));
}


TEST(Transform, DecomposeTransform)
{
    const Vector3 translation{ -1.0f, 2.5f, -3.75f };
    const Quaternion rotation = MakeTestRotation();
    const Vector3 scale{ 2.0f, 3.0f, 4.0f };
    const Matrix4x4 matrix = Matrix4x4::Scale(scale) * Matrix4x4::Rotation(rotation) * Matrix4x4::Translation(translation);

    Vector3 actualTranslation;
    Quaternion actualRotation;
    Vector3 actualScale;
    Vector3 actualShear;

    ASSERT_TRUE(Math::DecomposeTransform(matrix, actualTranslation, actualRotation, actualScale, actualShear));
    EXPECT_TRUE(Math::CmpEqual(actualTranslation, translation));
    EXPECT_TRUE(Math::CmpEqual(actualScale, scale, 1e-5f));
    EXPECT_TRUE(Math::CmpEqual(actualShear, Vector3::kZero, 1e-5f));
    ExpectSameRotation(actualRotation, rotation);
}
