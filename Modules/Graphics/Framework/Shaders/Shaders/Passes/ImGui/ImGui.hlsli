#pragma once
#include <Shaders/Passes/ImGui/ImGui.h>

struct PixelAttributes
{
    float4 m_position : SV_Position;
    float4 m_color : COLOR0;
    float2 m_texCoord : TEXCOORD0;
};

[[vk::push_constant]] DrawConstants GConstants;
