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
