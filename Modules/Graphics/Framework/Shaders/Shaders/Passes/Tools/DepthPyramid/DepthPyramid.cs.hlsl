#include <Shaders/Passes/Tools/DepthPyramid/DepthPyramid.h>

// Reduces 32x32 depth texels per group to mips 0-4 of the farthest (min) and closest (max) reverse-Z pyramids, following the
// LDS reduction of MiniEngine's DownsampleBloomCS. Mip zero is at least one group wide on both axes, so every thread maps to
// a texel of every mip it writes.

[[vk::constant_id(0)]] const bool WritesClosest = true;

[[vk::push_constant]] TileConstants GConstants;

groupshared float GFarthest[kTileGroupSize * kTileGroupSize];
groupshared float GClosest[kTileGroupSize * kTileGroupSize];


void StoreMip(const uint mip, const uint2 texel, const float farthest, const float closest)
{
    GConstants.m_farthest[mip].Store(texel, farthest);
    if (WritesClosest)
        GConstants.m_closest[mip].Store(texel, closest);
}


void StoreTile(const uint index, const float farthest, const float closest)
{
    GFarthest[index] = farthest;
    if (WritesClosest)
        GClosest[index] = closest;
}


// Combines the texel of this thread with the rest of its 2x2 block of the previous mip, stored in LDS at the given offsets.
void ReduceTile(const uint index, const uint3 offsets, inout float farthest, inout float closest)
{
    farthest = min(min(farthest, GFarthest[index + offsets.x]), min(GFarthest[index + offsets.y], GFarthest[index + offsets.z]));
    if (WritesClosest)
        closest = max(max(closest, GClosest[index + offsets.x]), max(GClosest[index + offsets.y], GClosest[index + offsets.z]));
}


FE_NUM_THREADS(kTileGroupSize, kTileGroupSize, 1)
void main(const uint index : SV_GroupIndex, const uint3 group : SV_GroupID, const uint3 texel : SV_DispatchThreadID)
{
    // The single-pass downsampler that runs next needs zeroed counters; the first lanes of the first group clear them.
    if (all(group.xy == 0) && index < kSpdCounterCount)
        GConstants.m_spdCounters.Store(index, 0);

    // Texels beyond the depth target clamp to its edge, so the padding only repeats on-screen depth.
    const float2 centerUV = (float2(texel.xy) * 2.0f + 1.0f) * GConstants.m_invDepthSize;
    Texture2D<float> depth = GConstants.m_depth.Get();
    const float4 quad = depth.Gather(GConstants.m_sampler.Get(), centerUV);
    float farthest = min(min(quad.x, quad.y), min(quad.z, quad.w));
    float closest = max(max(quad.x, quad.y), max(quad.z, quad.w));
    StoreMip(0, texel.xy, farthest, closest);
    StoreTile(index, farthest, closest);

    // The low bits of both coordinates tell whether this thread owns a block of the next mip.
    const uint parity = texel.x | texel.y;
    GroupMemoryBarrierWithGroupSync();
    if ((parity & 1) == 0)
    {
        ReduceTile(index, uint3(1, kTileGroupSize, kTileGroupSize + 1), farthest, closest);
        StoreMip(1, texel.xy >> 1, farthest, closest);
        StoreTile(index, farthest, closest);
    }

    GroupMemoryBarrierWithGroupSync();
    if ((parity & 3) == 0)
    {
        ReduceTile(index, uint3(2, kTileGroupSize * 2, kTileGroupSize * 2 + 2), farthest, closest);
        StoreMip(2, texel.xy >> 2, farthest, closest);
        StoreTile(index, farthest, closest);
    }

    GroupMemoryBarrierWithGroupSync();
    if ((parity & 7) == 0)
    {
        ReduceTile(index, uint3(4, kTileGroupSize * 4, kTileGroupSize * 4 + 4), farthest, closest);
        StoreMip(3, texel.xy >> 3, farthest, closest);
        StoreTile(index, farthest, closest);
    }

    GroupMemoryBarrierWithGroupSync();
    if ((parity & 15) == 0)
    {
        ReduceTile(index, uint3(8, kTileGroupSize * 8, kTileGroupSize * 8 + 8), farthest, closest);
        StoreMip(4, texel.xy >> 4, farthest, closest);
    }
}
