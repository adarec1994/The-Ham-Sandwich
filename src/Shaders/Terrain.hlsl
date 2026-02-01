struct FogParameters
{
    float3 color;
    float linearStart;
    float linearEnd;
    float density;
    int equation;
    int isEnabled;
};

struct SunParameters
{
    float3 color;
    float pad0;
    float3 direction;
    float intensity;
    int isEnabled;
    float3 pad1;
};

struct EnvironmentParameters
{
    float3 ambientColor;
    float pad0;
    FogParameters fogParams;
    SunParameters sunParams;
    int isEnabled;
    float3 pad1;
};

struct BrushParameters
{
    int mode;
    float3 position;
    float size;
    int isEnabled;
    float2 pad0;
};

struct TerrainEditorParameters
{
    int enableAreaGrid;
    int enableChunkGrid;
    float2 pad0;
    BrushParameters brushParams;
};

struct TerrainParameters
{
    float4 heightScale;
    float4 heightOffset;
    float4 parallaxScale;
    float4 parallaxOffset;
    float4 metersPerTextureTile;
    float specularPower;
    float2 scrollSpeed;
    int enableColorMap;
    int enableUnkMap2;
    float3 pad0;
};

cbuffer TerrainCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    TerrainParameters terrainParams;
    TerrainEditorParameters tEditorParams;
    EnvironmentParameters envParams;
};

Texture2D layer0 : register(t0);
Texture2D layer1 : register(t1);
Texture2D layer2 : register(t2);
Texture2D layer3 : register(t3);
Texture2D normal0 : register(t4);
Texture2D normal1 : register(t5);
Texture2D normal2 : register(t6);
Texture2D normal3 : register(t7);
Texture2D blendMap : register(t8);
Texture2D colorMap : register(t9);
Texture2D unkMap2 : register(t10);

SamplerState samplerWrap : register(s0);
SamplerState samplerClamp : register(s1);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 eyeSpacePosition : TEXCOORD3;
};

float getFogFactor(FogParameters params, float fogCoordinate)
{
    float result = 0.0;

    if (params.equation == 0)
    {
        float fogLength = params.linearEnd - params.linearStart;
        result = (params.linearEnd - fogCoordinate) / fogLength;
    }
    else if (params.equation == 1)
    {
        result = exp(-params.density * fogCoordinate);
    }
    else if (params.equation == 2)
    {
        result = exp(-pow(params.density * fogCoordinate, 2.0));
    }

    result = 1.0 - saturate(result);
    return result;
}

float3 UnpackWSNormal(float4 packednormal)
{
    float3 normal;
    normal.xy = packednormal.yw * 2.0 - 1.0;
    normal.z = sqrt(1.0 - normal.x * normal.x - normal.y * normal.y);
    return normal;
}

float dist(float2 p0, float2 pf)
{
    return sqrt((pf.x - p0.x) * (pf.x - p0.x) + (pf.y - p0.y) * (pf.y - p0.y));
}

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(float4(input.position, 1.0), model);
    output.fragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)model;
    output.normal = normalize(mul(input.normal, normalMatrix));

    output.uv = input.uv;

    float4 viewPos = mul(worldPos, view);
    output.eyeSpacePosition = viewPos;
    output.position = mul(viewPos, projection);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float3 albedo = float3(0.0, 0.0, 0.0);

    float2 tc0 = uv * (32.0 / terrainParams.metersPerTextureTile.x);
    float2 tc1 = uv * (32.0 / terrainParams.metersPerTextureTile.y);
    float2 tc2 = uv * (32.0 / terrainParams.metersPerTextureTile.z);
    float2 tc3 = uv * (32.0 / terrainParams.metersPerTextureTile.w);

    float4 normal0Sampler = normal0.Sample(samplerWrap, tc0);
    float4 normal1Sampler = normal1.Sample(samplerWrap, tc1);
    float4 normal2Sampler = normal2.Sample(samplerWrap, tc2);
    float4 normal3Sampler = normal3.Sample(samplerWrap, tc3);

    float height0 = normal0Sampler.r;
    float height1 = normal1Sampler.r;
    float height2 = normal2Sampler.r;
    float height3 = normal3Sampler.r;

    float4 blendSampler = blendMap.Sample(samplerClamp, uv);
    float4 colorSampler = colorMap.Sample(samplerClamp, uv);

    float2 parallax0 = float2(0.0, 0.0);
    float2 parallax1 = float2(0.0, 0.0);
    float2 parallax2 = float2(0.0, 0.0);
    float2 parallax3 = float2(0.0, 0.0);

    float3 norm0 = UnpackWSNormal(normal0Sampler);
    float3 norm1 = UnpackWSNormal(normal1Sampler);
    float3 norm2 = UnpackWSNormal(normal2Sampler);
    float3 norm3 = UnpackWSNormal(normal3Sampler);

    float4 layer0Sampler = layer0.Sample(samplerWrap, tc0 + parallax1);
    float4 layer1Sampler = layer1.Sample(samplerWrap, tc1 + parallax1);
    float4 layer2Sampler = layer2.Sample(samplerWrap, tc2 + parallax2);
    float4 layer3Sampler = layer3.Sample(samplerWrap, tc3 + parallax3);

    float4 layer_weights = float4(blendSampler.rgb, saturate(1.0 - blendSampler.r - blendSampler.g - blendSampler.b));
    float4 layer_pct = float4(
        layer_weights.x * (height0 * terrainParams.heightScale[0] + terrainParams.heightOffset[0]),
        layer_weights.y * (height1 * terrainParams.heightScale[1] + terrainParams.heightOffset[1]),
        layer_weights.z * (height2 * terrainParams.heightScale[2] + terrainParams.heightOffset[2]),
        layer_weights.w * (height3 * terrainParams.heightScale[3] + terrainParams.heightOffset[3])
    );

    float max1 = max(layer_pct.x, layer_pct.y);
    float max2 = max(layer_pct.z, layer_pct.w);
    float max3 = max(max1, max2);
    float4 layer_pct_max = float4(max3, max3, max3, max3);
    float4 scale = float4(1.0, 1.0, 1.0, 1.0) - saturate(layer_pct_max - layer_pct);
    layer_pct = layer_pct * scale;
    float sum2 = dot(float4(1.0, 1.0, 1.0, 1.0), layer_pct);
    layer_pct = layer_pct / float4(sum2, sum2, sum2, sum2);

    float4 weightedLayer_0 = layer0Sampler * layer_pct.x;
    float4 weightedLayer_1 = layer1Sampler * layer_pct.y;
    float4 weightedLayer_2 = layer2Sampler * layer_pct.z;
    float4 weightedLayer_3 = layer3Sampler * layer_pct.w;

    float4 weightedNormal_0 = normal0Sampler * layer_pct.x;
    float4 weightedNormal_1 = normal1Sampler * layer_pct.y;
    float4 weightedNormal_2 = normal2Sampler * layer_pct.z;
    float4 weightedNormal_3 = normal3Sampler * layer_pct.w;

    float metalBlend = weightedLayer_0.a + weightedLayer_1.a;
    float specBlend = weightedLayer_2.a + weightedLayer_3.a;

    float3 colorMapValue = colorSampler.rgb;
    if (!terrainParams.enableColorMap)
        colorMapValue = float3(0.5, 0.5, 0.5);
    float3 matDiffuse = (weightedLayer_0.rgb + weightedLayer_1.rgb + weightedLayer_2.rgb + weightedLayer_3.rgb) * colorMapValue * 2.0;
    albedo = matDiffuse.rgb;
    float3 normalBlend = (weightedNormal_0.rgb + weightedNormal_1.rgb + weightedNormal_2.rgb + weightedNormal_3.rgb);

    float3 finalNormal = input.normal;

    float depth = abs(input.eyeSpacePosition.z / input.eyeSpacePosition.w);

    if (envParams.sunParams.isEnabled)
    {
        float3 lightDir = normalize(envParams.sunParams.direction);
        float diffuse = max(dot(lightDir, finalNormal), 0.0);
        albedo = diffuse * envParams.sunParams.color * matDiffuse.rgb;
    }

    if (envParams.isEnabled)
    {
        float3 ambient = envParams.ambientColor * matDiffuse.rgb;
        albedo += ambient;
    }

    if (tEditorParams.brushParams.isEnabled)
    {
        float d = dist(tEditorParams.brushParams.position.xz, input.fragPos.xz) * (1.0 / tEditorParams.brushParams.size);
        float3 brushColor = float3(0.1, 0.1, 0.7);

        if (tEditorParams.brushParams.mode == 0)
        {
            float3 brushValue = lerp(brushColor, float3(0.0, 0.0, 0.0), saturate(d));
            albedo += brushValue;
        }
        else if (tEditorParams.brushParams.mode == 1)
        {
            float minDist = 0.99;
            float maxDist = minDist + (tEditorParams.brushParams.size * 0.0002);
            if (d > minDist && d < maxDist)
            {
                albedo += float3(1.0, 1.0, 1.0);
            }
        }
    }

    if (tEditorParams.enableAreaGrid)
    {
        float cthickness = 0.001;
        float2 wposMod = float2(fmod(32768.0 + input.fragPos.x, 512.0), fmod(32768.0 + input.fragPos.z, 512.0)) / 512.0;
        if (wposMod.x < cthickness || wposMod.x > 1 - cthickness || wposMod.y < cthickness || wposMod.y > 1 - cthickness)
        {
            albedo = lerp(float3(0.3, 0.3, 1.0), albedo.rgb, saturate(depth / 800));
        }
    }

    if (tEditorParams.enableChunkGrid)
    {
        float sthickness = 0.006;
        if (uv.x < sthickness || uv.x > 1 - sthickness || uv.y < sthickness || uv.y > 1 - sthickness)
        {
            albedo.rgb = lerp(saturate(albedo.rgb + 0.3), albedo.rgb, saturate(depth / 800.0));
        }
    }

    if (envParams.fogParams.isEnabled)
    {
        albedo = lerp(albedo, envParams.fogParams.color, getFogFactor(envParams.fogParams, depth));
    }

    return float4(albedo.rgb, 1.0);
}
