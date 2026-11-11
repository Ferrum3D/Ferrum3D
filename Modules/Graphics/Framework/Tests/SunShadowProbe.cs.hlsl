#include <Shaders/Tests/SunShadowProbe.h>
#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(kThreadGroupSize, kThreadGroupSize, 1)
void main(uint3 texel : SV_DispatchThreadID)
{
    if (any(texel.xy >= GConstants.m_size))
        return;

    if (GConstants.m_mode == kRequestMode)
    {
        const uint index = texel.y * GConstants.m_size.x + texel.x + GConstants.m_outputOffset;
        GConstants.m_requests.Store(index, GConstants.m_requests.Load(index) | kSunShadowPageRequested);
        return;
    }

    if (GConstants.m_mode == kFillAtlasMode)
    {
        const SunShadowData data = GConstants.m_shadow.m_data.Load();
        float depth;
        const uint2 center = uint2(SunShadow::ProjectTexel(data, GConstants.m_level, GConstants.m_position, depth));
        const int2 origin = data.m_levels.LoadElement(GConstants.m_level).m_pageOrigin;
        const uint entry = GConstants.m_shadow.m_pageTable.Load(SunShadow::GetPageIndex(GConstants.m_level, origin, center));
        if (SunShadow::IsBacked(entry))
            GConstants.m_atlas.Store(SunShadow::GetAtlasTexel(entry, texel.xy), asuint(0.9f));

        return;
    }

    if (GConstants.m_mode == kSampleMode)
    {
        GConstants.m_output.Store(0, SunShadow::SampleVisibility(GConstants.m_shadow, GConstants.m_position, float3(0, 1, 0)));
        return;
    }

    if (GConstants.m_mode == kFillMode)
    {
        GConstants.m_target.Store(texel.xy, texel.y < GConstants.m_backgroundRows ? 0.0f : GConstants.m_depth);
        return;
    }

    const float value = GConstants.m_source.Load(uint3(texel.xy, GConstants.m_level));
    GConstants.m_output.Store(GConstants.m_outputOffset + texel.y * GConstants.m_size.x + texel.x, value);
}
