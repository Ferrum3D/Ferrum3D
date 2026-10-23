#pragma once
#include <Shaders/Base/Packing.hlsli>

static const float3 kDebugBackgroundColor = float3(0.02f, 0.02f, 0.04f);


float3 DebugIDColor(uint id)
{
    id = (id ^ (id >> 16)) * 0x7feb352du;
    id = (id ^ (id >> 15)) * 0x846ca68bu;
    id ^= id >> 16;
    return Math::Pack::RGBA8UnormToRGBA32Float(id).rgb;
}


// LOD 0 is green; coarser LODs move through yellow and red to blue, and LODs beyond the palette repeat it darker.
float3 DebugLodColor(const uint lod)
{
    const float3 colors[8] = { float3(0.10f, 0.85f, 0.20f), float3(0.65f, 0.90f, 0.10f), float3(1.00f, 0.85f, 0.10f),
                               float3(1.00f, 0.50f, 0.05f), float3(0.95f, 0.15f, 0.10f), float3(0.90f, 0.15f, 0.70f),
                               float3(0.50f, 0.20f, 0.95f), float3(0.15f, 0.40f, 1.00f) };
    return colors[lod % 8] * (lod >= 8 ? 0.5f : 1.0f);
}
