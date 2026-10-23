#include <Shaders/Passes/Visibility/Debug.h>
#include <Shaders/Passes/Visibility/DebugCommon.hlsli>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>

[[vk::push_constant]] DebugConstants GConstants;


FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint2 id = GConstants.m_visibility.Load(uint3(pixel.xy, 0));
    const MeshViewData view = LoadMeshView(GConstants.m_view);
    if (id.x == 0 || (id.x >> kObjectTypeShift) != view.m_scene.Load(0).m_visibilityType)
        return;

    float3 color = kDebugBackgroundColor;
    if (GConstants.m_mode == DebugMode::kLod)
    {
        // Shade with the reconstructed normal so the silhouette and simplification of each LOD stay visible.
        ReconstructedAttributes attributes;
        const bool reconstructed =
            ReconstructAttributes(view, id, float2(pixel.xy) + 0.5f, float2(GConstants.m_resolution), attributes, false, false);
        const float lighting =
            reconstructed ? 0.35f + 0.65f * saturate(dot(attributes.m_normal, normalize(float3(0.4f, 0.8f, -0.45f)))) : 1.0f;
        color = DebugLodColor(DecodeLod(id.y)) * lighting;
    }
    else if (GConstants.m_mode <= DebugMode::kTriangle)
    {
        const uint key = GConstants.m_mode == DebugMode::kInstance ? id.x
            : GConstants.m_mode == DebugMode::kMeshlet             ? id.y >> kPrimitiveBits
            : GConstants.m_mode == DebugMode::kTriangle            ? DecodePrimitive(id.y)
                                                                   : id.x * 65537 + id.y;
        color = DebugIDColor(key);
    }
    else
    {
        ReconstructedAttributes attributes;
        if (ReconstructAttributes(view, id, float2(pixel.xy) + 0.5f, float2(GConstants.m_resolution), attributes))
        {
            if (GConstants.m_mode == DebugMode::kUV)
                color = float3(frac(attributes.m_uv), 0);
            else if (GConstants.m_mode == DebugMode::kNormal)
                color = attributes.m_normal * 0.5f + 0.5f;
            else if (GConstants.m_mode == DebugMode::kTangent)
                color = attributes.m_tangent.xyz * 0.5f + 0.5f;
            else if (GConstants.m_mode == DebugMode::kDerivatives)
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

    GConstants.m_output.Store(pixel.xy, color);
}
