//--------------------------------------------------------------------------------------
// File: Tutorial06.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

cbuffer ConstantBufferTransforms : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    matrix mat[65];
}

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 weights : BLENDWEIGHTS;
    int4 indices : BLENDINDICES;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD1;
    float4 world_pos : POSITIONT;
    float4 eye_pos : EYEPOS;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    float4 skinnedPos = {0.0f,0.0f,0.0f,0.0f };
    float4 skinnedNorm = { 0.0f,0.0f,0.0f,0.0f };
    for (int i = 0; i < 4;i++)
    {
        skinnedPos += mul(float4(input.Pos.xyz, 1.0f), mat[input.indices[i]] * input.weights[i]);
        skinnedNorm += mul(float4(input.Norm.xyz, 0.0f), mat[input.indices[i]] * input.weights[i]);
    
    }
    output.world_pos = mul(float4(skinnedPos.xyz, 1.0f), World);
	output.Pos = mul(output.world_pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.eye_pos.x = -dot(View[3].xyz, View[0].xyz);
	output.eye_pos.y = -dot(View[3].xyz, View[1].xyz);
	output.eye_pos.z = -dot(View[3].xyz, View[2].xyz);
	output.eye_pos.w = 1.0f;
    output.Norm = mul(float4(skinnedNorm.xyz, 0.0f), World);
	output.Tex = input.Tex.xy;
	return output;
}
