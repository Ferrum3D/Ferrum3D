#pragma once

namespace Shading
{
    struct Surface
    {
        float3 m_diffuseAlbedo;
        float3 m_f0;
        float m_perceptualRoughness;
        float3 m_worldSpaceNormal;
    };
} // namespace Shading
