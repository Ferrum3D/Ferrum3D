#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Passes/Visibility/DebugCommon.hlsli>
#include <Shaders/Lighting/LocalLightTable.hlsli>
#include <Shaders/Shadows/LocalShadow.hlsli>

[[vk::push_constant]] SunShadowDebugConstants GConstants;

static const float3 kNoLocalShadowColor = float3(0.15f, 0.25f, 0.6f);


float3 GetLocalShadowColor(const float3 worldPosition, const float3 normal, const float2 pixel)
{
    const LocalLightingBinding lighting = GConstants.m_localLighting;
    const LocalShadow::Camera camera = LocalShadow::LoadCamera(GConstants.m_shadow);
    if (!lighting.m_clusters.IsValid() || !camera.m_isValid)
        return kNoLocalShadowColor;

    const LightClusterData clusters = lighting.m_clusters.Load(0);
    const uint base = GetLightClusterBase(clusters, worldPosition, pixel);
    float visibility = 2.0f;
    for (uint word = 0; word < clusters.m_wordCount; ++word)
    {
        uint mask = lighting.m_masks.Load(base + word);
        while (mask != 0)
        {
            const uint bit = firstbitlow(mask);
            mask &= ~(1u << bit);
            const uint lightMaps = LocalShadow::LoadLightMaps(GConstants.m_shadow, word * 32 + bit);
            if (lightMaps == kLocalShadowNoMap)
                continue;

            // Only positions the light reaches count.
            const DecodedLocalLight light = LoadLocalLight(lighting, word * 32 + bit);
            const float3 delta = worldPosition - light.m_positionRange.xyz;
            const float distance = length(delta);
            const bool isInCone = light.m_directionOuterCos.w <= -1
                || dot(delta / max(distance, 0.0001f), light.m_directionOuterCos.xyz) > light.m_directionOuterCos.w;
            if (distance >= light.m_positionRange.w || !isInCone)
                continue;

            visibility = min(visibility,
                             LocalShadow::SampleVisibility(GConstants.m_shadow, camera, lightMaps, worldPosition, normal));
        }
    }

    return visibility > 1.0f ? kNoLocalShadowColor : (float3)visibility;
}


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
    else if (GConstants.m_mode == kSunShadowDebugLocalFactor)
    {
        color = GetLocalShadowColor(worldPosition, normal, float2(pixel) + 0.5f);
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
            // Cached pages show as their own color with dark borders, pages drawn this frame as yellow and unbacked pages as red.
            const uint2 texel = uint2(projection.m_texel);
            const int2 pageOrigin = data.m_levels[projection.m_level].m_pageOrigin;
            const uint entry = GConstants.m_shadow.m_pageTable.Load(SunShadow::GetPageIndex(projection.m_level, pageOrigin, texel));
            const uint2 inPage = texel % kSunShadowPageSize;
            const bool isBorder = any(inPage < 2) || any(inPage >= kSunShadowPageSize - 2);
            if ((entry & kSunShadowPageAllocated) == 0)
                color = float3(1, 0, 0);
            else if ((entry & kSunShadowPageDirty) != 0)
                color = float3(1, 0.9f, 0.1f);
            else
                color = DebugIDColor(entry & kSunShadowPhysicalPageMask);

            color *= isBorder ? 0.25f : 1.0f;
        }
    }

    GConstants.m_output.Store(pixel, color);
}
