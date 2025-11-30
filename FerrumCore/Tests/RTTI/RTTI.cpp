#include <FeCore/Compression/Compression.h>
#include <FeCore/IO/FileStream.h>
#include <FeCore/Math/Transform.h>
#include <FeCore/RTTI/Reflection.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(RTTI, BuiltinTypes)
{
    const RTTI::Type& type = RTTI::GetType<uint32_t>();
    EXPECT_EQ(type.m_name, "uint32_t");
    EXPECT_EQ(type.m_qualifiedName, "uint32_t");
    EXPECT_EQ(type.m_id, RTTI::GetTypeID<uint32_t>());
    EXPECT_EQ(&type, RTTI::TypeRegistry::FindType(type.m_id));
    EXPECT_EQ(&type, RTTI::TypeRegistry::FindType("uint32_t"));

    EXPECT_EQ(type.m_size, sizeof(uint32_t));
    EXPECT_EQ(type.m_alignment, alignof(uint32_t));

    EXPECT_TRUE(type.m_fields.empty());
}

TEST(RTTI, DynamicCast)
{
    IO::FileStream derived;
    IO::StreamBase* base0 = &derived;
    IO::BufferedStream* base1 = &derived;
    IO::IStream* base2 = &derived;

    EXPECT_EQ(RTTI::Cast<IO::StreamBase*>(base2), base0);
    EXPECT_EQ(RTTI::Cast<IO::FileStream*>(base2), &derived);
    EXPECT_EQ(RTTI::Cast<IO::FileStream*>(base0), base2);
    EXPECT_EQ(RTTI::Cast<IO::BufferedStream*>(base2), base1);

    EXPECT_EQ(RTTI::Cast<IO::IStream*>(&derived), base2);
    EXPECT_EQ(RTTI::Cast<IO::StreamBase*>(&derived), base0);
    EXPECT_EQ(RTTI::Cast<IO::IStream*>(base0), base2);
    EXPECT_EQ(RTTI::Cast<IO::IStream*>(base2), base2);
}

TEST(RTTI, Enum)
{
    const RTTI::Type& type = RTTI::GetType<Compression::Method>();
    EXPECT_EQ(type.m_name, "Method");
    EXPECT_EQ(type.m_qualifiedName, "FE::Compression::Method");
    EXPECT_EQ(type.m_id, RTTI::GetTypeID<Compression::Method>());
    EXPECT_EQ(&type, RTTI::TypeRegistry::FindType(type.m_id));
    EXPECT_EQ(&type, RTTI::TypeRegistry::FindType("FE::Compression::Method"));

    EXPECT_EQ(type.m_size, sizeof(Compression::Method));
    EXPECT_EQ(type.m_alignment, alignof(Compression::Method));

    EXPECT_EQ(type.m_enumNames[0], "kNone");
    EXPECT_EQ(type.m_enumNames[1], "kDeflate");
    EXPECT_EQ(type.m_enumNames[2], "kGDeflate");
    EXPECT_EQ(type.m_enumNames[3], "kInvalid");

    EXPECT_EQ(type.m_enumDisplayNames[0], "kNone");
    EXPECT_EQ(type.m_enumDisplayNames[1], "Deflate");
    EXPECT_EQ(type.m_enumDisplayNames[2], "GDeflate");
    EXPECT_EQ(type.m_enumDisplayNames[3], "Invalid");

    EXPECT_EQ(type.m_enumValues[0], festd::to_underlying(Compression::Method::kNone));
    EXPECT_EQ(type.m_enumValues[1], festd::to_underlying(Compression::Method::kDeflate));
    EXPECT_EQ(type.m_enumValues[2], festd::to_underlying(Compression::Method::kGDeflate));
    EXPECT_EQ(type.m_enumValues[3], festd::to_underlying(Compression::Method::kInvalid));

    EXPECT_TRUE(type.m_fields.empty());

    const RTTI::TypeID underlyingTypeID = festd::single(type.m_baseTypes);
    const RTTI::Type* underlyingType = RTTI::TypeRegistry::FindType(underlyingTypeID);
    EXPECT_NE(underlyingType, nullptr);

    EXPECT_EQ(underlyingType, &RTTI::GetType<std::underlying_type_t<Compression::Method>>());
    EXPECT_EQ(underlyingType->m_id, RTTI::GetTypeID<std::underlying_type_t<Compression::Method>>());
    EXPECT_EQ(underlyingType->m_size, type.m_size);
}

TEST(RTTI, Reflection)
{
    const RTTI::Type& type = RTTI::GetType<Transform>();
    EXPECT_EQ(type.m_name, "Transform");
    EXPECT_EQ(type.m_qualifiedName, "FE::Transform");
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
