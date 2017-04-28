struct VSInput
{
	float3  Position : POSITION;
};

struct VSOutput
{
	float4  Position : SV_POSITION;
	float4  Color    : VTX_COLOR;
};

struct PSOutput
{
	float4  Color   : SV_TARGET0;
};


cbuffer TransformBuffer : register(b0)
{
	float4x4 World : packoffset(c0);
	float4x4 View  : packoffset(c4);
	float4x4 Proj  : packoffset(c8);
};

