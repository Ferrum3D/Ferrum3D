struct VSOutput
{
    float4 pos: SV_POSITION;
    float4 color : COLOR;
};

float4 main(VSOutput input) : SV_TARGET
{
    return input.color;
}
