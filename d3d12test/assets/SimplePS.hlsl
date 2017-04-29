#include "SimpleDef.hlsli"

PSOutput PSFunc(const VSOutput input)
{
	PSOutput output = (PSOutput)0;

	float3 N = normalize(input.Normal);
	float3 L = float3(0.0, 1.0, 0.0);

	float3 light = max(dot(N, L), 0.0) * 0.5 + 0.5;
	output.Color = float4(light * light, 1.0);

	output.Color = float4(input.Texture0, 0.0, 1.0);

	return output;
}

