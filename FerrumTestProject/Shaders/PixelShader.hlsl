struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

// Vulkan binding 0 in descriptor set 0
cbuffer Settings : register(b0, space0)
{
    float4 sColor;
};

float4 main(VSOutput input) : SV_TARGET
{
    return sColor * input.color;
}
