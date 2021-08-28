struct VSOutput
{
    float4 pos: SV_POSITION;
};

// Vulkan binding 0 in descriptor set 0
cbuffer Settings : register(b0, space0)
{
    float4 color;
};

float4 main(VSOutput input) : SV_TARGET
{
    return color;
}
