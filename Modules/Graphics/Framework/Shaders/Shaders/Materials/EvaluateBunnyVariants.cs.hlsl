#include <Shaders/Materials/EvaluateMesh.hlsli>

FE_NUM_THREADS(EVALUATION_LANES * kSurfaceWavesPerGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 dispatch : SV_GroupID)
{
    uint2 pixel;
    uint groupIndex;
    if (!LoadEvaluationPixel(dispatch, lane, pixel, groupIndex))
        return;

    const MeshViewData view = LoadMeshView(GConstants.m_view);
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));

    MeshEvaluationContext context;
    if (!LoadMeshEvaluation(view, id, pixel, GConstants.m_resolution, groupIndex, context, false, false))
    {
        GConstants.m_g0.Store(pixel, 0xffff00ffu);
        return;
    }

#if BUNNY_PALETTE == 1
    const float3 color = float3(0.25f, 0.7f, 0.9f);
#else
    const float3 color = float3(0.95f, 0.4f, 0.2f);
#endif

    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = color;
    surface.m_normal = context.m_attributes.m_normal;
    surface.m_shadingRoute = context.m_shadingRoute;
    StoreGBuffer(pixel, surface);
}
