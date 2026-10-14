#include <Shaders/Tests/GpuProbe.h>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>

[[vk::push_constant]] Constants GConstants;


FE_NUM_THREADS(1, 1, 1)
void main(uint3 group : SV_GroupID)
{
    float4 a = float4(-1, -1, 0.5f, 1);
    float4 b = float4(2, -2, 0.5f, 2);
    float4 c = float4(0, 4, 0.5f, 4);
    if (group.x == 1)
        a.w = 0;
    else if (group.x == 2)
        a.w = -1;
    else if (group.x == 3)
        c = b;

    const float2 pixel = float2(0.1f, -0.2f);
    const float2 step = float2(2.0f / 1280, -2.0f / 720);
    PerspectiveWeights weights;
    const bool valid = ComputePerspectiveWeights(a, b, c, pixel, step, weights);
    PerspectiveWeights x0, x1, y0, y1;
    const float epsilon = 0.0625f;
    ComputePerspectiveWeights(a, b, c, pixel - float2(step.x, 0) * epsilon, step, x0);
    ComputePerspectiveWeights(a, b, c, pixel + float2(step.x, 0) * epsilon, step, x1);
    ComputePerspectiveWeights(a, b, c, pixel - float2(0, step.y) * epsilon, step, y0);
    ComputePerspectiveWeights(a, b, c, pixel + float2(0, step.y) * epsilon, step, y1);
    const float3 finiteX = (x1.m_value - x0.m_value) / (2 * epsilon);
    const float3 finiteY = (y1.m_value - y0.m_value) / (2 * epsilon);
    const uint offset = group.x * 12;
    GConstants.m_words.Store(offset, valid ? 1 : 0);
    for (uint lane = 0; lane < 3; ++lane)
    {
        GConstants.m_words.Store(offset + 1 + lane, asuint(weights.m_value[lane]));
        GConstants.m_words.Store(offset + 4 + lane, asuint(weights.m_ddx[lane]));
        GConstants.m_words.Store(offset + 7 + lane, asuint(weights.m_ddy[lane]));
    }

    GConstants.m_words.Store(offset + 10, asuint(max(max(abs(weights.m_ddx - finiteX).x, abs(weights.m_ddx - finiteX).y), abs(weights.m_ddx - finiteX).z)));
    GConstants.m_words.Store(offset + 11, asuint(max(max(abs(weights.m_ddy - finiteY).x, abs(weights.m_ddy - finiteY).y), abs(weights.m_ddy - finiteY).z)));
}
