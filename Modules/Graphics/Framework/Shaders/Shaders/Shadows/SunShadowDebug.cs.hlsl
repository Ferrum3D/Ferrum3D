#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Passes/Visibility/DebugCommon.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowDebugConstants GConstants;

FE_NUM_THREADS(kSunShadowTileSize, kSunShadowTileSize, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint2 pixel = thread.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    const float depth = GConstants.m_depth.Load(uint3(pixel, 0));
    if (depth <= 0.0f)
    {
        GConstants.m_output.Store(pixel, kDebugBackgroundColor);
        return;
    }

    const ViewData view = GConstants.m_view.Load(0);
    const float2 viewportPosition = (float2(pixel) + 0.5f - view.m_viewport.xy) / view.m_viewport.zw;
    const float2 ndc = viewportPosition * float2(2, -2) + float2(-1, 1);
    const float4 homogeneousWorld = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
    const float3 worldPosition = homogeneousWorld.xyz / homogeneousWorld.w;
    const float3 normal = Math::Pack::Octahedral16SnormToNormal(GConstants.m_g1.Load(uint3(pixel, 0)));

    float3 color = float3(1, 1, 1);
    if (GConstants.m_mode == kSunShadowDebugFactor)
    {
        color = SunShadow::SampleVisibility(GConstants.m_shadow, worldPosition, normal);
    }
    else if (GConstants.m_shadow.m_data.IsValid())
    {
        const SunShadowData data = GConstants.m_shadow.m_data.Load(0);
        const SunShadow::Projection projection = SunShadow::Project(data, worldPosition);
        if (!projection.m_isValid)
        {
            color = kDebugBackgroundColor;
        }
        else if (GConstants.m_mode == kSunShadowDebugLevel)
        {
            color = DebugLodColor(projection.m_level);
        }
        else
        {
            // Pages show as their own color with dark borders; unbacked pages are red.
            const uint2 texel = uint2(projection.m_texel);
            const uint entry = GConstants.m_shadow.m_pageTable.Load(SunShadow::GetPageIndex(projection.m_level, texel));
            const uint2 inPage = texel % kSunShadowPageSize;
            const bool isBorder = any(inPage < 2) || any(inPage >= kSunShadowPageSize - 2);
            color = (entry & kSunShadowPageAllocated) != 0 ? DebugIDColor(entry & kSunShadowPhysicalPageMask) : float3(1, 0, 0);
            color *= isBorder ? 0.25f : 1.0f;
        }
    }

    GConstants.m_output.Store(pixel, color);
}
