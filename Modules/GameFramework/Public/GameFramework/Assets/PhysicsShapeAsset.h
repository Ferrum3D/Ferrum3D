#pragma once
#include <Core/IO/Assets.h>
#include <Core/Math/Quaternion.h>

namespace FE::GameFramework
{
    enum class PhysicsShapeType : uint32_t
    {
        kSphere,
        kBox,
        kCapsule,
        kCylinder,
    };


    //! @brief Source-less primitive collision asset. Dimensions are in meters, along local Y for capsules and cylinders.
    struct PhysicsShapeAsset final
    {
        PhysicsShapeType m_type = PhysicsShapeType::kSphere;
        Vector3 m_halfExtent{ 0.5f, 0.5f, 0.5f };
        float m_radius = 0.5f;
        //! @brief Half the cylindrical section, excluding capsule end caps.
        float m_halfHeight = 0.5f;

        FE_RTTI_Reflect("3A4A6717-42D0-4393-9C32-A84949024167");
        FE_RTTI_Serialize();
    };


    struct PhysicsBodyShape final
    {
        //! @brief Optional shared definition; otherwise use the inline primitive.
        IO::Link<PhysicsShapeAsset> m_asset;
        PhysicsShapeAsset m_shape;
        Vector3 m_position = Vector3::kZero;
        Quaternion m_rotation = Quaternion::kIdentity;

        FE_RTTI_Reflect("F6EA422D-7667-42E3-96D8-B8A5EEF7CAFF");
        FE_RTTI_Serialize();
    };
} // namespace FE::GameFramework
