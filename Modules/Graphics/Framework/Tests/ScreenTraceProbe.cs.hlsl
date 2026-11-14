#include <Shaders/Shadows/ScreenTrace.hlsli>
#include <Shaders/Tests/ScreenTraceProbe.h>

[[vk::push_constant]] Constants GConstants;


uint Hash(const uint2 texel, const uint seed)
{
    uint hash = (texel.x * 73856093u) ^ (texel.y * 19349663u) ^ (seed * 83492791u);
    hash ^= hash >> 13;
    hash *= 0x5bd1e995u;
    hash ^= hash >> 15;
    return hash;
}


float HashUnorm(const uint2 texel, const uint seed)
{
    return float(Hash(texel, seed) & 0xffffu) * (1.0f / 65536.0f);
}


// The trace as it was before its steps interpolated clip coordinates: every step is projected, and the visible surface is
// unprojected to measure how far the step lies behind it.
bool ReferenceIsOccluded(const Texture2DDescriptor<float> depth, const ViewData view, const float3 origin,
                         const float3 direction, const float rayLength, const float bias, const uint stepCount)
{
    Texture2D<float> depthTexture = depth.Get();
    for (uint step = 0; step < stepCount; ++step)
    {
        const float3 position = origin + direction * (rayLength * float(step + 1) / float(stepCount));
        const float4 clip = mul(float4(position, 1.0f), view.m_viewProjection);
        if (clip.w <= 0.0f)
            return false;

        const float2 ndc = clip.xy / clip.w;
        if (any(abs(ndc) >= 1.0f))
            return false;

        const float2 pixel = (ndc * float2(0.5f, -0.5f) + 0.5f) * view.m_viewport.zw + view.m_viewport.xy;
        const float sceneDepth = depthTexture.Load(int3(pixel, 0));
        if (sceneDepth <= 0.0f)
            continue;

        const float4 sceneWorld = mul(float4(ndc, sceneDepth, 1.0f), view.m_inverseViewProjection);
        const float sceneDistance = length(sceneWorld.xyz / sceneWorld.w - view.m_cameraPosition.xyz);
        const float behind = length(position - view.m_cameraPosition.xyz) - sceneDistance;
        if (behind > bias && behind < rayLength)
            return true;
    }

    return false;
}


FE_NUM_THREADS(kThreadGroupSize, kThreadGroupSize, 1)
void main(uint3 texel : SV_DispatchThreadID)
{
    if (any(texel.xy >= GConstants.m_size))
        return;

    if (GConstants.m_mode == kFillMode)
    {
        // Blocks of constant depth with large steps between them, and some background.
        const uint2 block = texel.xy / 6;
        const uint level = Hash(block, 0) % 8;
        GConstants.m_target.Store(texel.xy, level == 0 ? 0.0f : 0.02f + 0.004f * float(level));
        return;
    }

    const ViewData view = GConstants.m_view.Load();
    const float2 ndc = ((float2(texel.xy) + 0.5f - view.m_viewport.xy) / view.m_viewport.zw) * float2(2, -2) + float2(-1, 1);
    const float depth = max(GConstants.m_depth.Load(uint3(texel.xy, 0)), 0.02f);
    const float4 world = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
    const float3 origin = world.xyz / world.w;
    const float distance = length(origin - view.m_cameraPosition.xyz);

    // Rays in all directions that cross several blocks, with a bias of about a pixel.
    const float3 randomDirection = float3(HashUnorm(texel.xy, 1), HashUnorm(texel.xy, 2), HashUnorm(texel.xy, 3)) * 2.0f - 1.0f;
    const float3 direction = normalize(randomDirection + 1e-3f);
    const float rayLength = distance * (0.02f + 0.3f * HashUnorm(texel.xy, 4));
    const float bias = distance * 0.002f;
    const uint stepCount = 4 + Hash(texel.xy, 5) % 5;

    const ScreenTrace::Context trace = ScreenTrace::CreateContext(GConstants.m_depth, view);
    uint result = 0;
    if (ScreenTrace::IsOccluded(trace, origin, direction, rayLength, bias, stepCount))
        result |= kTraceOccluded;
    if (ReferenceIsOccluded(GConstants.m_depth, view, origin, direction, rayLength, bias, stepCount))
        result |= kReferenceOccluded;

    GConstants.m_output.Store(texel.y * GConstants.m_size.x + texel.x, result);
}
