struct VSOutput
{
	float4 pos : SV_Position;
	float3 color : COLOR;
};


float4 ps_main(VSOutput input) : SV_Target0
{
	return float4(input.color, 1.0f);
}
