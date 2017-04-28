#include "SimpleDef.hlsli"

PSOutput PSFunc(const VSOutput input)
{
    PSOutput output = (PSOutput)0;

    output.Color = input.Color;

    return output;
}

