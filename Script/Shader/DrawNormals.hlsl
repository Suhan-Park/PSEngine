//***************************************************************************************
// DrawNormals.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Common.hlsl"

struct VertexIn
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentU : TANGENT;
	float4 BoneWeights : WEIGHTS;
	int4 BoneIndices : BONEINDICES;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
	if (gSkinningFlag)
	{
		float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		weights[0] = vin.BoneWeights.x;
		weights[1] = vin.BoneWeights.y;
		weights[2] = vin.BoneWeights.z;
		weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

		float3 posL = float3(0.0f, 0.0f, 0.0f);
		float3 normalL = float3(0.0f, 0.0f, 0.0f);
		float3 tangentL = float3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 4; ++i)
		{
			posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
			normalL += weights[i] * mul(vin.NormalL, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
			tangentL += weights[i] * mul(vin.TangentU.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
		}

		vin.PosL = posL;
		vin.NormalL = normalL;
		vin.TangentU.xyz = tangentL;
	}

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = texC.xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo = DiffuseAlbedo;

	// Diffuse Map을 샘플링한다.
	if (DiffuseMapFlag)
	{
		diffuseAlbedo *= gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
	}

#ifdef ALPHA_TEST
    // Discard pixel if texture alpha < 0.1.  We do this test as soon 
    // as possible in the shader so that we can potentially exit the
    // shader early, thereby skipping the rest of the shader code.
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);
	
    // NOTE: We use interpolated vertex normal for SSAO.

    // Write normal in view space coordinates
    float3 normalV = mul(pin.NormalW, (float3x3)gView);
    return float4(normalV, 0.0f);
}


