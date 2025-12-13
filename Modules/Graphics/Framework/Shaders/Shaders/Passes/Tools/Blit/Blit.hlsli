#pragma once
#include <Shaders/Passes/Tools/Blit/Blit.h>

struct PixelAttributes
{
    float4 m_position : SV_Position;
    float2 m_texCoord : TEXCOORD0;
};

[[vk::push_constant]] Constants GConstants;
