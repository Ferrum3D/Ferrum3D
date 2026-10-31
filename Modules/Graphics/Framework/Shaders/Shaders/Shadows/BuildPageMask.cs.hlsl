#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowPageMaskConstants GConstants;

FE_NUM_THREADS(kSunShadowTileSize, kSunShadowTileSize, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint2 texel = thread.xy;
    if (any(texel >= kSunShadowMaskSize))
        return;

    const uint2 virtualTexel = texel / kSunShadowMaskTexelsPerPage * kSunShadowPageSize;
    const uint entry = GConstants.m_pageTable.Load(SunShadow::GetPageIndex(GConstants.m_level, GConstants.m_pageOrigin, virtualTexel));
    const bool isDrawn = (entry & GConstants.m_requiredFlags) == GConstants.m_requiredFlags;
    GConstants.m_mask.Store(texel, isDrawn ? 0.0f : 1.0f);
}
