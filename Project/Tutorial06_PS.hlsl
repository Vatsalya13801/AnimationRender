//--------------------------------------------------------------------------------------
// File: Tutorial06.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D tx_diffuse : register(t0);
Texture2D tx_emissive : register(t1);
Texture2D tx_specular : register(t2);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    float4 vLightDir[2];
    float4 vLightColor[2];
    float4 vOutputColor;
}


//--------------------------------------------------------------------------------------
//struct VS_INPUT
//{
//    float4 Pos : POSITION;
//    float3 Norm : NORMAL;
//    float2 Tex : TEXCOORD0;
//};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD1;
	float4 world_pos : POSITIONT;
	float4 eye_pos : EYEPOS;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
    
    //do NdotL lighting for 2 lights
   // for (int i = 0; i < 2; i++)
   // {
   //     finalColor += saturate(dot((float3) vLightDir[i], input.Norm) * vLightColor[i]);
   // }
   // //Alpha type 0 use 0.75f;
   // //Alpha type 1 and 2 use 1.0f;
   // finalColor.a = 1.0f;
   //// finalColor *= txDiffuse.Sample(samLinear, input.Tex);
    

    float3 light_dir = vLightDir[1];
    float sq_dist = dot(light_dir, light_dir);

    light_dir = light_dir / sqrt(sq_dist);
	float3 eye_dir = input.eye_pos.xyz - input.world_pos.xyz;
	float sq_dist_eye = dot(eye_dir, eye_dir);
	float distance_eye = sqrt(sq_dist_eye);
	eye_dir = eye_dir / distance_eye;
	float3 norm = normalize(input.Norm.xyz);
	float nl = dot(norm, light_dir);
	float diffuse_intensity = saturate(nl);
	float3 half_vector = normalize(light_dir + eye_dir);
	float nh = dot(norm, half_vector);
	float specular_intensity = pow(saturate(nh), 1 + 4.0f);
	float4 light_intensity = float4(vLightColor[1]) * 0.5f / sq_dist;
	float4 mat_diffuse = tx_diffuse.Sample(samLinear, input.Tex); // *float4(surface_diffuse, 0.0f) * surface_diffuse_factor;
	float4 mat_specular = tx_specular.Sample(samLinear, input.Tex); // *float4(surface_specular, 0.0f) * surface_specular_factor;
	float4 mat_emissive = tx_emissive.Sample(samLinear, input.Tex); // *float4(surface_emissive, 0.0f) * surface_emissive_factor;
	float4 emissive = mat_emissive;
	float4 ambient = mat_diffuse * 0.1f;
	float4 specular = mat_specular * specular_intensity * light_intensity;
	float4 diffuse = mat_diffuse * diffuse_intensity * light_intensity;
    // hacky conservation of energy
	diffuse.xyz -= specular.xyz;
	diffuse.xyz = saturate(diffuse.xyz);
	float4 color = ambient + specular + diffuse + emissive;
        
	return color;

}