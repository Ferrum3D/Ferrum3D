#include <Shaders/Passes/Tools/DepthPyramid/DepthPyramid.h>

// A single-channel port of the wave-intrinsic path of AMD FidelityFX SPD (MIT license, Copyright (C) 2024 Advanced Micro
// Devices, Inc.). Each group reduces a 64x64 tile of the source to 1x1 in six mips; the last group to finish reduces the
// mid mip it left behind to 1x1. Mip numbers here are output indices: output 0 has half the source size.

// The number of mips written below the source, and whether the pass reduces to the farthest (min) or closest (max) depth.
[[vk::constant_id(0)]] const uint MipCount = kSpdMaxMipCount;
[[vk::constant_id(1)]] const bool Farthest = true;

[[vk::push_constant]] SpdConstants GConstants;

groupshared float GIntermediate[16][16];
groupshared uint GCounter;


// Reverse-Z: the farthest depth is the smallest one.
float Reduce4(const float v0, const float v1, const float v2, const float v3)
{
    if (Farthest)
        return min(min(v0, v1), min(v2, v3));

    return max(max(v0, v1), max(v2, v3));
}


float ReduceQuad(const float value)
{
    return Reduce4(value, QuadReadAcrossX(value), QuadReadAcrossY(value), QuadReadAcrossDiagonal(value));
}


uint2 GetOutputSize(const uint mip)
{
    return max(GConstants.m_sourceSize >> (mip + 1), 1u);
}


void Store(const uint2 texel, const float value, const uint mip)
{
    if (any(texel >= GetOutputSize(mip)))
        return;

    if (mip == kSpdMidMip)
        GConstants.m_midMip.Store(texel, value);
    else
        GConstants.m_mips[mip].Store(texel, value);
}


// The clamp sampler repeats the edge of non-square sources, so reductions above them only combine covered texels.
float LoadSource2x2(const uint2 texel)
{
    Texture2D<float> source = GConstants.m_source.Get();
    const float4 quad = source.Gather(GConstants.m_sampler.Get(), (float2(texel) + 1.0f) * GConstants.m_invSourceSize);
    return Reduce4(quad.x, quad.y, quad.z, quad.w);
}


float LoadMid2x2(const uint2 texel)
{
    const uint2 lastTexel = GetOutputSize(kSpdMidMip) - 1;
    const float v0 = GConstants.m_midMip.Load(min(texel, lastTexel));
    const float v1 = GConstants.m_midMip.Load(min(texel + uint2(1, 0), lastTexel));
    const float v2 = GConstants.m_midMip.Load(min(texel + uint2(0, 1), lastTexel));
    const float v3 = GConstants.m_midMip.Load(min(texel + uint2(1, 1), lastTexel));
    return Reduce4(v0, v1, v2, v3);
}


// Places the 64 threads of a wave on an 8x8 block so that consecutive quads cover 2x2 texel blocks.
uint2 RemapForWaveReduction(const uint index)
{
    return uint2(Bit::FieldInsert(Bit::FieldExtract(index, 2, 3), index, 0, 1),
                 Bit::FieldInsert(Bit::FieldExtract(index, 3, 3), Bit::FieldExtract(index, 1, 2), 0, 2));
}


void DownsampleMips01(const uint x, const uint y, const uint2 workGroup, const uint index)
{
    float values[4];
    [unroll] for (uint quadrant = 0; quadrant < 4; ++quadrant)
    {
        const uint2 offset = uint2(quadrant % 2, quadrant / 2) * 16;
        const uint2 texel = workGroup * 32 + uint2(x, y) + offset;
        values[quadrant] = LoadSource2x2(texel * 2);
        Store(texel, values[quadrant], 0);
    }

    if (MipCount <= 1)
        return;

    [unroll] for (uint quadrant = 0; quadrant < 4; ++quadrant) values[quadrant] = ReduceQuad(values[quadrant]);

    if (index % 4 != 0)
        return;

    [unroll] for (uint quadrant = 0; quadrant < 4; ++quadrant)
    {
        const uint2 texel = uint2(x / 2, y / 2) + uint2(quadrant % 2, quadrant / 2) * 8;
        Store(workGroup * 16 + texel, values[quadrant], 1);
        GIntermediate[texel.x][texel.y] = values[quadrant];
    }
}


void DownsampleMip2(const uint x, const uint y, const uint2 workGroup, const uint index, const uint mip)
{
    const float value = ReduceQuad(GIntermediate[x][y]);
    if (index % 4 == 0)
    {
        Store(workGroup * 8 + uint2(x / 2, y / 2), value, mip);
        GIntermediate[x + (y / 2) % 2][y] = value;
    }
}


void DownsampleMip3(const uint x, const uint y, const uint2 workGroup, const uint index, const uint mip)
{
    if (index >= 64)
        return;

    const float value = ReduceQuad(GIntermediate[x * 2 + y % 2][y * 2]);
    if (index % 4 == 0)
    {
        Store(workGroup * 4 + uint2(x / 2, y / 2), value, mip);
        GIntermediate[x * 2 + y / 2][y * 2] = value;
    }
}


void DownsampleMip4(const uint x, const uint y, const uint2 workGroup, const uint index, const uint mip)
{
    if (index >= 16)
        return;

    const float value = ReduceQuad(GIntermediate[x * 4 + y][y * 4]);
    if (index % 4 == 0)
    {
        Store(workGroup * 2 + uint2(x / 2, y / 2), value, mip);
        GIntermediate[x / 2 + y][0] = value;
    }
}


void DownsampleMip5(const uint2 workGroup, const uint index, const uint mip)
{
    if (index >= 4)
        return;

    const float value = ReduceQuad(GIntermediate[index][0]);
    if (index % 4 == 0)
        Store(workGroup, value, mip);
}


void DownsampleNextFour(const uint x, const uint y, const uint2 workGroup, const uint index, const uint baseMip)
{
    if (MipCount <= baseMip)
        return;

    GroupMemoryBarrierWithGroupSync();
    DownsampleMip2(x, y, workGroup, index, baseMip);
    if (MipCount <= baseMip + 1)
        return;

    GroupMemoryBarrierWithGroupSync();
    DownsampleMip3(x, y, workGroup, index, baseMip + 1);
    if (MipCount <= baseMip + 2)
        return;

    GroupMemoryBarrierWithGroupSync();
    DownsampleMip4(x, y, workGroup, index, baseMip + 2);
    if (MipCount <= baseMip + 3)
        return;

    GroupMemoryBarrierWithGroupSync();
    DownsampleMip5(workGroup, index, baseMip + 3);
}


void DownsampleMips67(const uint x, const uint y)
{
    float values[4];
    [unroll] for (uint quadrant = 0; quadrant < 4; ++quadrant)
    {
        const uint2 offset = uint2(quadrant % 2, quadrant / 2);
        values[quadrant] = LoadMid2x2(uint2(x, y) * 4 + offset * 2);
        Store(uint2(x, y) * 2 + offset, values[quadrant], 6);
    }

    if (MipCount <= 7)
        return;

    // Each thread already holds a 2x2 block of mip 6.
    const float value = Reduce4(values[0], values[1], values[2], values[3]);
    Store(uint2(x, y), value, 7);
    GIntermediate[x][y] = value;
}


// Only the last group to finish its tile continues with the mips below the mid mip.
bool IsLastWorkGroup(const uint index)
{
    if (index == 0)
        InterlockedAdd(GConstants.m_atomicCounter.Get()[GConstants.m_counterIndex], 1, GCounter);

    GroupMemoryBarrierWithGroupSync();
    return GCounter == GConstants.m_workGroupCount - 1;
}


FE_NUM_THREADS(256, 1, 1)
void main(const uint index : SV_GroupIndex, const uint3 workGroup : SV_GroupID)
{
    const uint2 waveTexel = RemapForWaveReduction(index % 64);
    const uint x = waveTexel.x + 8 * ((index >> 6) % 2);
    const uint y = waveTexel.y + 8 * (index >> 7);
    DownsampleMips01(x, y, workGroup.xy, index);
    DownsampleNextFour(x, y, workGroup.xy, index, 2);
    if (MipCount <= kSpdMidMip + 1)
        return;

    if (!IsLastWorkGroup(index))
        return;

    DownsampleMips67(x, y);
    DownsampleNextFour(x, y, uint2(0, 0), index, 8);
}
