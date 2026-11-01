#pragma once
#include <Core/Math/Frustum.h>
#include <Core/Math/Transform.h>
#include <Graphics/Features/Lighting/LocalLightingSceneModule.h>
#include <Shaders/Shadows/SunShadow.h>
#include <festd/base.h>

namespace FE::Graphics::LocalShadows
{
    //! Shadow maps of one camera view. A spot light takes one map and a point light six, one per cube face, so the cap allows
    //! e.g. sixteen spot lights or two point lights and four spot lights.
    constexpr uint32_t kMaxMapCount = kLocalShadowMaxMapCount;


    //! The light a map belongs to and the perspective projection it is rendered with.
    struct MapSetup final
    {
        LightHandle m_light;
        //! The light state the cached pages of the map were drawn for.
        LocalLightData m_lightData{};
        //! Cube face of a point light, ordered +X, -X, +Y, -Y, +Z, -Z; zero for spot lights.
        uint32_t m_face = 0;
        uint32_t m_faceCount = 0;
        Transform m_camera{ kForceInit };
        float m_fovY = 0.0f;
        float m_near = 0.0f;
        float m_far = 0.0f;
        Matrix4x4 m_viewProjection = Matrix4x4::kIdentity;
        //! World size of a texel of mip 0 per unit of distance along the view direction.
        float m_texelScale = 0.0f;
        bool m_isUsed = false;

        [[nodiscard]] bool IsFirstFace() const
        {
            return m_isUsed && m_face == 0;
        }
    };


    //! Maps assigned to lights; kept across frames so that lights keep their cached pages.
    struct MapAllocation final
    {
        festd::array<MapSetup, kMaxMapCount> m_maps;
    };


    struct CameraDesc final
    {
        Frustum m_frustum{ Matrix4x4::kIdentity };
        Vector3 m_position = Vector3::kZero;
    };


    //! Assign maps to the shadow casting lights that the camera sees, the lights that cover most of the screen first. Lights
    //! keep their maps while they stay selected. Returns a bit per map whose cached pages are no longer valid: maps that
    //! were released, assigned to another light or whose light changed.
    uint32_t SelectMaps(const LocalLightingSceneModule& lights, const CameraDesc& camera, MapAllocation& allocation);


    //! Shader data of a map whose page table blocks start at firstBlock.
    [[nodiscard]] LocalShadowMapData BuildMapData(const MapSetup& map, uint32_t firstBlock);
} // namespace FE::Graphics::LocalShadows
