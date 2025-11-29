#include <FeCore/Math/Transform.h>
#include <FeCore/RTTI/Reflection.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(RTTI, Reflection)
{
    const RTTI::Type& type = RTTI::GetType<Transform>();
    EXPECT_EQ(type.m_name, "Transform");
    EXPECT_EQ(type.m_id, RTTI::GetTypeID<Transform>());
    EXPECT_EQ(&type, RTTI::TypeRegistry::FindType(type.m_id));
    EXPECT_EQ(&type, RTTI::TypeRegistry::FindType("FE::Transform"));

    EXPECT_EQ(type.m_fields[0].m_name, "m_translationScale");
    EXPECT_EQ(type.m_fields[1].m_name, "m_rotation");

    EXPECT_EQ(type.m_fields[0].m_type, RTTI::GetTypeID<Vector4>());
    EXPECT_EQ(type.m_fields[1].m_type, RTTI::GetTypeID<Quaternion>());

    EXPECT_EQ(type.m_fields[0].m_size, sizeof(Vector4));
    EXPECT_EQ(type.m_fields[1].m_size, sizeof(Quaternion));

    EXPECT_EQ(type.m_fields[0].m_offset, offsetof(Transform, m_translationScale));
    EXPECT_EQ(type.m_fields[1].m_offset, offsetof(Transform, m_rotation));

    EXPECT_EQ(type.m_size, sizeof(Transform));
    EXPECT_EQ(type.m_alignment, alignof(Transform));

    Transform transform;
    type.m_fields[0].Set(&transform, Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    type.m_fields[1].Set(&transform, Quaternion::RotationX(1.0f));

    EXPECT_EQ(transform.m_translationScale, Vector4(1.0f, 2.0f, 3.0f, 4.0f));
    EXPECT_EQ(transform.m_rotation, Quaternion::RotationX(1.0f));
}
