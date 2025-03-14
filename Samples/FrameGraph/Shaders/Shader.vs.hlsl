struct VSOutput
{
	float4 pos : SV_Position;
	float3 color : COLOR;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 color : COLOR;
};


VSOutput vs_main(const in VSInput input)
{
	VSOutput output;
	output.pos = float4(input.pos, 1.0f);
	output.color = input.color;
	return output;
}
