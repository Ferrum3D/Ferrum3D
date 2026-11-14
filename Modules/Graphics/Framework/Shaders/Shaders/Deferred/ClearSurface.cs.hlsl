#include <Shaders/Deferred/Surface.h>

[[vk::push_constant]] SurfaceConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (all(pixel.xy < GConstants.m_work.m_resolution))
    {
        const uint instance = GConstants.m_visibility.Load(uint3(pixel.xy, 0)).x;
        GConstants.m_g0.Store(pixel.xy, instance == 0 ? 0 : 0xffff00ffu);
        if (!GConstants.m_motion.IsValid())
            return;

        if (!GConstants.m_camera.IsValid())
        {
            GConstants.m_motion.Store(pixel.xy, 0);
            return;
        }

        const ViewData view = GConstants.m_camera.Load();
        const float2 uv = (float2(pixel.xy) + 0.5f) / float2(GConstants.m_work.m_resolution);
        const float4 farPosition = mul(float4(uv * float2(2, -2) + float2(-1, 1), 0, 1), view.m_inverseViewProjection);
        const float3 direction = farPosition.xyz - view.m_cameraPosition.xyz * farPosition.w;
        const float4 current = mul(float4(direction, 0), view.m_unjitteredViewProjection);
        const float4 previous = mul(float4(direction, 0), view.m_previousViewProjection);
        const float2 motion =
            current.w > 0 && previous.w > 0 ? (previous.xy / previous.w - current.xy / current.w) * float2(0.5f, -0.5f) : 0;
        GConstants.m_motion.Store(pixel.xy, motion);
    }
}
