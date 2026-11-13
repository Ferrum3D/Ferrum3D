#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

// Without render targets, the nearest reverse-Z depth of each backed texel is kept with an atomic maximum. Non-negative floats
// order like their bit patterns, so the comparison works on the raw bits.
void main(float4 position : SV_Position, nointerpolation uint viewIndex : VIEW)
{
    const SunShadowRasterData data = GPass.m_passData.As<SunShadowRasterData>().Load();
    const SunShadowRasterView view = data.m_views.LoadElement(viewIndex);

    // Triangles are clipped to the viewport of the pass, which is larger than the views of the coarser local map mips.
    const uint2 texel = uint2(position.xy);
    if (any(texel >= view.m_size))
        return;

    const uint entry = data.m_pageTable.Load(SunShadow::GetPageIndex(view.m_level, view.m_pageOrigin, texel));
    if ((entry & data.m_requiredFlags) != data.m_requiredFlags)
        return;

    RWTexture2D<uint> atlas = data.m_atlas.Get();
    const uint2 target = SunShadow::GetAtlasTexel(entry, texel);
    const uint depth = asuint(saturate(position.z));
    if (atlas[target] < depth)
        InterlockedMax(atlas[target], depth);
}
