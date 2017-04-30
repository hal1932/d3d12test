struct VSInput
{
	float3 Position : POSITION;
	float3 Normal   : NORMAL;
	float2 Texture0 : TEXTURE0;
};

struct VSOutput
{
	float4 Position : SV_POSITION;
	float3 Normal   : NORMAL;
	float2 Texture0 : TEXTURE0;
};

struct PSOutput
{
	float4 Color   : SV_TARGET0;
};


cbuffer TransformBuffer : register(b0)
{
	float4x4 World : packoffset(c0);
	float4x4 View  : packoffset(c4);
	float4x4 Proj  : packoffset(c8);
};

Texture2D ColorTexture : register(t0);
SamplerState ColorSampler : register(s0);
