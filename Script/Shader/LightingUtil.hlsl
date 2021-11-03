//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 15
#define MaxDirectionLightCount 3
#define MaxPointLightCount 6
#define MaxSpotLightCount 6

struct LightData
{
    float3 Position;
    float  Pad1;
    float3 Direction;
    float  Pad2;
    float4 Diffuse;
    float  Range;
    float  Intensity;
    float  SpotAngle;
    float  Pad3;
};

struct Material
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

float CalcAttenuation(float d, float range)
{
    return saturate((range - d) / range);
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightDiffuse, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
	const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
    
	float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightDiffuse;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(LightData L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightDiffuse = (L.Diffuse * ndotl).rgb;

    return BlinnPhong(lightDiffuse, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(LightData L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.Range)
        return float3(0.0f, 0.0f, 0.0f);

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightDiffuse = (L.Diffuse * ndotl).rgb;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.Range);
    lightDiffuse *= att;

    return BlinnPhong(lightDiffuse, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(LightData L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.Range)
		return float3(0.0f, 0.0f, 0.0f);

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightDiffuse = (L.Diffuse * ndotl).rgb;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.Range);
    lightDiffuse *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotAngle);
    lightDiffuse *= spotFactor;

    return BlinnPhong(lightDiffuse, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(LightData gLights[MaxLights],
                       int gDirectionalLightCount, int gPointLightCount, int gSpotLightCount,
                       Material mat,
                       float3 pos, float3 normal, float3 toEye, float shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

    if (gDirectionalLightCount > 0)
    {
        for (i = 0; i < gDirectionalLightCount; ++i)
        {
            result += shadowFactor * gLights[i].Intensity * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
        }
    }

    if (gPointLightCount > 0)
    {
        for (i = MaxDirectionLightCount; i < MaxDirectionLightCount + gPointLightCount; ++i)
        {
            result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
        }
    }

    if (gSpotLightCount > 0)
    {
        for (i = MaxDirectionLightCount + MaxSpotLightCount; i < MaxDirectionLightCount + MaxSpotLightCount + gSpotLightCount; ++i)
        {
            result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
        }
    }

    return float4(result, 0.0f);
}