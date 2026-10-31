#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

// Without render targets, the nearest reverse-Z depth of each backed texel is kept with an atomic maximum. Non-negative floats
// order like their bit patterns, so the comparison works on the raw bits.
void main(float4 position : SV_Position)
{
    const SunShadowRasterData data = GView.m_passData.Load<SunShadowRasterData>(0);
    const uint2 texel = uint2(position.xy);
    const uint entry = data.m_pageTable.Load(SunShadow::GetPageIndex(data.m_level, texel));
    if ((entry & kSunShadowPageAllocated) == 0)
        return;

    RWTexture2D<uint> atlas = data.m_atlas.Get();
    InterlockedMax(atlas[SunShadow::GetAtlasTexel(entry, texel)], asuint(saturate(position.z)));
}
