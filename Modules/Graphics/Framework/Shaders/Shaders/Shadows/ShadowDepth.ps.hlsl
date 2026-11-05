#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

// Without render targets, the nearest reverse-Z depth of each backed texel is kept with an atomic maximum. Non-negative floats
// order like their bit patterns, so the comparison works on the raw bits.
void main(float4 position : SV_Position)
{
    const SunShadowRasterData data = GPass.m_passData.As<SunShadowRasterData>().Load();
    const uint2 texel = uint2(position.xy);
    const uint entry = data.m_pageTable.Load(SunShadow::GetPageIndex(data.m_level, data.m_pageOrigin, texel));
    if ((entry & data.m_requiredFlags) != data.m_requiredFlags)
        return;

    RWTexture2D<uint> atlas = data.m_atlas.Get();
    InterlockedMax(atlas[SunShadow::GetAtlasTexel(entry, texel)], asuint(saturate(position.z)));
}
