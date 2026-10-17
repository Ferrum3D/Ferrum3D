#include <Shaders/Passes/Visibility/Debug.h>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>

[[vk::push_constant]] DebugConstants GConstants;


float3 IDColor(uint value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return float3(value & 255, (value >> 8) & 255, (value >> 16) & 255) / 255.0f;
}


FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint2 id = GConstants.m_visibility.Load(uint3(pixel.xy, 0));
    const MeshViewData view = LoadMeshView(GConstants.m_view);
    if (id.x != 0 && (id.x >> kObjectTypeShift) != view.m_scene.Load(0).m_visibilityType)
        return;

    float3 color = float3(0.02f, 0.02f, 0.04f);
    if (id.x != 0)
    {
        if (GConstants.m_mode <= 4)
        {
            const uint key = GConstants.m_mode == 2 ? id.x
                : GConstants.m_mode == 3            ? id.y >> kPrimitiveBits
                : GConstants.m_mode == 4            ? id.y & (kMaxPrimitiveCount - 1)
                                                    : id.x * 65537 + id.y;
            color = IDColor(key);
        }
        else
        {
            ReconstructedAttributes attributes;
            if (ReconstructAttributes(view, id, float2(pixel.xy) + 0.5f, float2(GConstants.m_resolution), attributes))
            {
                if (GConstants.m_mode == 5)
                    color = float3(frac(attributes.m_uv), 0);
                else if (GConstants.m_mode == 6)
                    color = attributes.m_normal * 0.5f + 0.5f;
                else if (GConstants.m_mode == 7)
                    color = attributes.m_tangent.xyz * 0.5f + 0.5f;
                else if (GConstants.m_mode == 8)
                    color = float3(abs(attributes.m_uvDdx) * 32.0f, length(attributes.m_uvDdy) * 32.0f);
                else
                {
                    const float rasterDepth = GConstants.m_depth.Load(uint3(pixel.xy, 0));
                    color = float3(saturate(abs(attributes.m_depth - rasterDepth) * 100000.0f), 0, 0);
                }
            }
            else
                color = float3(1, 0, 1);
        }
    }

    GConstants.m_output.Store(pixel.xy, color);
}
