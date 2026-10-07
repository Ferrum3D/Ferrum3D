#pragma once
#include <Core/Math/Matrix4x4.h>
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::GameFramework
{
    struct TransformComponent final
    {
        Matrix4x4 m_local = Matrix4x4::kIdentity;
        FE_RTTI_Reflect("a8cdb00d-20ce-4780-b08d-211d0f600001");
        FE_RTTI_Serialize();
    };


    struct NonUniformScaleComponent final
    {
        Vector3 m_scale{ 1.0f, 1.0f, 1.0f };
        FE_RTTI_Reflect("a8cdb00d-20ce-4780-b08d-211d0f600002");
        FE_RTTI_Serialize();
    };


    struct WorldTransformComponent final
    {
        Matrix4x4 m_world = Matrix4x4::kIdentity;
        FE_RTTI_Reflect("a8cdb00d-20ce-4780-b08d-211d0f600003");
    };
} // namespace FE::GameFramework
