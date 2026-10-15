#include <Shaders/Deferred/DeferredCommon.hlsli>

FE_NUM_THREADS(8, 8, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint2 pixel = dispatch.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    const uint g0 = GConstants.m_g0.Load(uint3(pixel, 0));
    const uint route = GBuffer::GetShadingRoute(g0);
    const bool unsupported = route == kUnsupportedShadingRoute || route > GConstants.m_shaderCount;
    if (route == kBackgroundShadingRoute || unsupported)
    {
        const DeferredViewData view = GConstants.m_view.Load(0);
        const float3 color = unsupported ? float3(1, 0, 1) : view.m_backgroundColor.rgb;
        GConstants.m_output.Store(pixel, color);
    }
}
