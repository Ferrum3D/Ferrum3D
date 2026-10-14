#include <Shaders/Passes/Visibility/EvaluateCommon.hlsli>

FE_NUM_THREADS(EVALUATION_LANES, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 dispatch : SV_GroupID)
{
    uint2 pixel;
    uint groupIndex;
    if (!LoadEvaluationPixel(dispatch, lane, pixel, groupIndex))
        return;

    const ViewData view = GConstants.m_view.Load(0);
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    ReconstructedAttributes attributes;
    if (!ReconstructAttributes(view, id, float2(pixel) + 0.5f, float2(GConstants.m_resolution), attributes, false, false))
    {
        GConstants.m_g0.Store(pixel, 0xffff00ffu);
        return;
    }

#if BUNNY_PALETTE == 1
    const float3 color = float3(0.25f, 0.7f, 0.9f);
#else
    const float3 color = float3(0.95f, 0.4f, 0.2f);
#endif
    StoreSurface(pixel, color, attributes.m_normal, 0, 0, 0, 0, SurfaceShadingRoute(view, groupIndex));
}
