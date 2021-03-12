#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only  
    float FalloffEnd;   // point/spot light only  
    float3 Position;    // point light only  
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float  Shininess;
};

float3 ToonDiffuse(float3 dif)
{
    float3 dc;
    if (dif.x <= 0)  dc.x = .4f;
    if (dif.y > .0 && dif.y <= .5) dc.y = .6f;
    if (dif.z >= .5f && dif.z <= 1.) dc.z = 1.f;
    return dc;
}

float3 ToonSpecular(float3 spec)
{
    float3 sc;
    if (spec.x <= 0.1)  sc.x = .0f;
    if (spec.y > .1f && spec.y <= .8) sc.y = .5f;
    if (spec.z > .8f && spec.z <= 1.) sc.z = .8f;
    return sc;
}

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffStart - d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.")
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    float f0 = 1.f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec,
                  float3 normal, float3 toEye, Material material)
{
    const float m = material.Shininess * 256.f;
    float3 halfVec = normalize(toEye + lightVec);

    float  roughnessFactor = (m + 8.f) * pow(max(dot(halfVec, normal), .0f), m) / 8.f;
    float3 fresnelFactor  = SchlickFresnel(material.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;
    specAlbedo = ToonSpecular(specAlbedo);
    material.DiffuseAlbedo.rgb = ToonSpecular(material.DiffuseAlbedo.rgb);

    // Our spec formula goes outside [0, 1] range, but we are
    // doing LDR rendering. So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.f);
    
    return (material.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light light, Material material, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -light.Direction;
    
    // Scale light down by Lambert's cosine law.
    float  ndotl = max(dot(lightVec, normal), .0f);
    float3 lightStrength = light.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light light, Material material, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = light.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test
    if (d > light.FalloffEnd)
        return .0f; 

    // Normalize the light vector
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float  ndotl = max(dot(lightVec, normal), .0f);
    float3 lightStrength = light.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, light.FalloffStart, light.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light light, Material material, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = light.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > light.FalloffEnd)
        return 0.f;
    
    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float  ndotl = max(dot(lightVec, normal), .0f);
    float3 lightStrength = light.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, light.FalloffStart, light.FalloffEnd);
    lightStrength *= att;

    // Scale by spolight
    float spotFactor = pow(max(dot(-lightVec, light.Direction), .0f), light.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, material);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS;
        i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}