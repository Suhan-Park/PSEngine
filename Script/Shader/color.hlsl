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
	float4 PosH  : SV_POSITION;
	float4 ShadowPosH : POSITION0;
	float4 SsaoPosH   : POSITION1;
	float3 PosW  : POSITION2;
	float3 NormalW : NORMAL;
	float2 TexC : TEXCOORD;
	float3 TangentW : TANGENT;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
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

	vout.PosW = posW.xyz;
	vout.PosH = mul(posW, gViewProj);
	vout.SsaoPosH = mul(posW, gViewProjTex);
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);
	vout.ShadowPosH = mul(posW, gShadowTransform);

	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = texC.xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Rasterize를 거친 후 보간된 Normal 값이 크기가 1을 벗어날 수 있기에, 다시 정규화를 해준다.
	pin.NormalW = normalize(pin.NormalW);
	pin.TexC *= Tiling;
	
	float4 diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
	float3 normal = pin.NormalW;

	// 카메라로부터 정점까지 방향을 계산한다.
	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Material 구조체를 구성한다. 
	Material mat = { DiffuseAlbedo, FresnelR0, Shininess };
	// Diffuse Map, Normal Map을 샘플링한다.
	if (DiffuseMapFlag)
	{
		diffuse = gDiffuseMap.Sample(gsamPointWrap, pin.TexC);
	}

	if (NormalMapFlag)
	{
		float3 normalMap = gNormalMap.Sample(gsamPointWrap, pin.TexC);
		// Tangent Space에 있는 법선을 World로 변환한다.
		normal = NormalSampleToWorldSpace(normalMap.rgb, pin.NormalW, pin.TangentW);
	}

	pin.SsaoPosH /= pin.SsaoPosH.w;
	float ambientAccess = gSSAOMap.Sample(gsamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;

	float shadowFactor = 1.0f;
	shadowFactor = CalcShadowFactor(pin.ShadowPosH);

	// 빛을 계산한다.
	float4 light = ComputeLighting(gLights,
		gDirectionalLightCount, gPointLightCount, gSpotLightCount, mat, pin.PosW, normal, toEyeW, shadowFactor);

	diffuse *= (DiffuseAlbedo * Shininess) + (DiffuseAlbedo * light);
	float ambient = ambientAccess * gAmbientLight * Ambient;
	float4 col = diffuse + ambient + (light * Shininess);
	
	col.a = DiffuseAlbedo.a;
	return col;
}