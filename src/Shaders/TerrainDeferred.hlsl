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
};

Texture2D textureLayer0 : register(t0);
Texture2D textureLayer1 : register(t1);
Texture2D textureLayer2 : register(t2);
Texture2D textureLayer3 : register(t3);
Texture2D textureNormal0 : register(t4);
Texture2D textureNormal1 : register(t5);
Texture2D textureNormal2 : register(t6);
Texture2D textureNormal3 : register(t7);
Texture2D textureBlendMap0 : register(t8);
Texture2D textureColorMap : register(t9);
Texture2D textureBlendMap1 : register(t10);

SamplerState samplerWrap : register(s0);
SamplerState samplerClamp : register(s1);

struct VSInput
{
    float3 position : POSITION;
    float4 normal : NORMAL;
    float v2 : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 tangent : TANGENT;
    float4 bitangent : BINORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float4 normal : TEXCOORD1;
    float v2 : TEXCOORD2;
    float2 uv : TEXCOORD3;
    float4 eyeSpacePosition : TEXCOORD4;
    float4 tangent : TEXCOORD5;
    float4 bitangent : TEXCOORD6;
};

struct PSOutput
{
    float4 diffuse : SV_TARGET0;
    float4 specular : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 unknown : SV_TARGET3;
};

float dist(float2 p0, float2 pf)
{
    return sqrt((pf.x - p0.x) * (pf.x - p0.x) + (pf.y - p0.y) * (pf.y - p0.y));
}

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(float4(input.position, 1.0), model);
    output.fragPos = worldPos.xyz;
    output.normal = input.normal;
    output.v2 = input.v2;
    output.uv = input.uv;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;

    float4 viewPos = mul(worldPos, view);
    output.eyeSpacePosition = viewPos;
    output.position = mul(viewPos, projection);

    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    float3 inPosition = input.fragPos;
    float4 inNormal = input.normal;
    float v2 = input.v2;
    float2 inUV = input.uv;
    float4 inTangent = input.tangent;
    float4 inBiTangent = input.bitangent;

    float4 specularAdjust = float4(2.0, 3.0, 0.0, 0.0);
    float4 colorMapAdjustment = float4(2.0, 1.0, 0.0, 0.0);

    float4 metersPerTexA = float4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.x, 0.0);
    float4 metersPerTexB = float4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.y, 0.0);
    float4 metersPerTexC = float4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.z, 0.0);
    float4 metersPerTexD = float4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.w, 0.0);

    float4 layer0SpecularAdjust = float4(0, 0, 0, 0);
    float4 layer1SpecularAdjust = float4(0, 0, 0, 0);
    float4 layer2SpecularAdjust = float4(0, 0, 0, 0);
    float4 layer3SpecularAdjust = float4(0, 0, 0, 0);

    float4 heightScale = float4(terrainParams.heightScale[0], terrainParams.heightScale[1], terrainParams.heightScale[2], terrainParams.heightScale[3]);
    float4 heightOffset = float4(terrainParams.heightOffset[0], terrainParams.heightOffset[1], terrainParams.heightOffset[2], terrainParams.heightOffset[3]);

    float4 parallaxScale = float4(
        terrainParams.parallaxScale[0] / 100.0,
        terrainParams.parallaxScale[1] / 100.0,
        terrainParams.parallaxScale[2] / 100.0,
        terrainParams.parallaxScale[3] / 100.0
    );
    float4 parallaxOffset = float4(
        -terrainParams.parallaxOffset[0] / 100.0,
        -terrainParams.parallaxOffset[1] / 100.0,
        -terrainParams.parallaxOffset[2] / 100.0,
        -terrainParams.parallaxOffset[3] / 100.0
    );

    float2 colorMapinUV = inUV.xy;
    float4 colorMapSample = textureColorMap.Sample(samplerClamp, colorMapinUV);
    float4 adjustedColorMap = colorMapSample * colorMapAdjustment.xxxy + colorMapAdjustment.zzzw;

    float2 layer0inUV = inUV.xy * metersPerTexA.zz + metersPerTexA.xy;
    float4 layer0 = textureLayer0.Sample(samplerWrap, layer0inUV);
    float4 normal0 = textureNormal0.Sample(samplerWrap, layer0inUV);
    float specularIntensityLayer0 = layer0SpecularAdjust.w * layer0.w;
    float3 specularValueLayer0 = layer0.xyz * specularIntensityLayer0.xxx;
    float3 diffuseLayer0 = -layer0.xyz * specularIntensityLayer0.xxx + layer0.xyz;
    float invSpecularIntensityLayer0 = -layer0.w * layer0SpecularAdjust.w + layer0.w;
    float3 specularColorLayer0 = layer0SpecularAdjust.xyz * specularValueLayer0.xyz;

    float2 layer1inUV = inUV.xy * metersPerTexB.zz + metersPerTexB.xy;
    float4 layer1 = textureLayer1.Sample(samplerWrap, layer1inUV);
    float4 normal1 = textureNormal1.Sample(samplerWrap, layer1inUV);
    float specularIntensityLayer1 = layer1SpecularAdjust.w * layer1.w;
    float3 specularValueLayer1 = layer1.xyz * specularIntensityLayer1.xxx;
    float3 diffuseLayer1 = -layer1.xyz * specularIntensityLayer1.xxx + layer1.xyz;
    float invSpecularIntensityLayer1 = -layer1.w * layer1SpecularAdjust.w + layer1.w;
    float3 specularColorLayer1 = layer1SpecularAdjust.xyz * specularValueLayer1.xyz;

    float2 layer2inUV = inUV.xy * metersPerTexC.zz + metersPerTexC.xy;
    float4 layer2 = textureLayer2.Sample(samplerWrap, layer2inUV);
    float4 normal2 = textureNormal2.Sample(samplerWrap, layer2inUV);
    float specularIntensityLayer2 = layer2SpecularAdjust.w * layer2.w;
    float3 specularValueLayer2 = specularIntensityLayer2.xxx * layer2.xyz;
    float3 diffuseLayer2 = -layer2.xyz * specularIntensityLayer2.xxx + layer2.xyz;
    float invSpecularIntensityLayer2 = -layer2.w * layer2SpecularAdjust.w + layer2.w;
    float3 specularColorLayer2 = layer2SpecularAdjust.xyz * specularValueLayer2;

    float2 layer3inUV = inUV.xy * metersPerTexD.zz + metersPerTexD.xy;
    float4 normal3 = textureNormal3.Sample(samplerWrap, layer3inUV);
    float4 layer3 = textureLayer3.Sample(samplerWrap, layer3inUV);
    float specularIntensityLayer3 = layer3SpecularAdjust.w * layer3.w;
    float3 specularValueLayer3 = layer3.xyz * specularIntensityLayer3.xxx;
    float3 diffuseLayer3 = -layer3.xyz * specularIntensityLayer3.xxx + layer3.xyz;
    float invSpecularIntensityLayer3 = -layer3.w * layer3SpecularAdjust.w + layer3.w;
    float3 specularColorLayer3 = layer3SpecularAdjust.xyz * specularValueLayer3;

    float2 blendMap1inUV = inUV.xy;
    float4 blendMap1 = textureBlendMap1.Sample(samplerClamp, blendMap1inUV);
    float3 scaledBlendMap1 = blendMap1.xyz * float3(2.0666666, 2.03225803, 2.0666666) + float3(-1, -1, -1);

    float2 blendMap0inUV = inUV.xy;
    float4 blendMap0 = textureBlendMap0.Sample(samplerClamp, blendMap0inUV);
    float4 blendMapMixed;
    blendMapMixed.xyz = blendMap0.xyz + scaledBlendMap1;
    float blendMapDot = dot(blendMapMixed.xyz, float3(1, 1, 1));
    blendMapMixed.w = 1 + -blendMapDot;
    blendMapMixed = saturate(blendMapMixed);
    blendMapMixed *= blendMap0.wwww;
    blendMapDot = dot(blendMapMixed, float4(1, 1, 1, 1));

    float4 layerHeightMaps = float4(normal0.x, normal1.x, normal2.x, normal3.x);
    float4 layerHeightMapsScaled = layerHeightMaps * heightScale + heightOffset;
    float4 layerHeightMapsBlended = blendMapMixed * layerHeightMapsScaled + float4(0.00100000005, 0.00100000005, 0.00100000005, 0.00100000005);
    float4 layerHeightMapsBlendPow = layerHeightMapsBlended * layerHeightMapsBlended;
    float heightMapsDot = dot(layerHeightMapsBlended, layerHeightMapsBlended);
    float4 heightXblend = layerHeightMapsBlendPow * blendMapDot.xxxx;
    float blendMapDotInv = 1 + -blendMapDot;
    float4 layerMasks = heightXblend / heightMapsDot.xxxx;

    float3 specColorCombined = (layerMasks.xxx * specularColorLayer0) + (layerMasks.yyy * specularColorLayer1) + (layerMasks.zzz * specularColorLayer2) + (layerMasks.www * specularColorLayer3);
    float3 specularColor = specColorCombined * adjustedColorMap.xyz;

    float3 diffuseCombined = (layerMasks.xxx * diffuseLayer0) + (layerMasks.yyy * diffuseLayer1.xyz) + (layerMasks.zzz * diffuseLayer2) + (layerMasks.www * diffuseLayer3);
    diffuseCombined += blendMapDotInv.xxx * float3(0.5, 0.5, 0.5);
    float3 diffuse = diffuseCombined * adjustedColorMap.xyz + specularColor;

    output.normal.z = adjustedColorMap.w;

    float specColorDot = dot(specularColor, float3(0.212500006, 0.715399981, 0.0720999986));
    float diffuseDot = dot(diffuse, float3(0.212500006, 0.715399981, 0.0720999986));

    output.diffuse.xyz = diffuse;

    float scaledDiffuseDot = 0.00100000005 + diffuseDot;
    output.diffuse.w = specColorDot / scaledDiffuseDot;

    float roughnessMasked = (layerMasks.x * normal0.z) + (layerMasks.y * normal1.z) + (layerMasks.z * normal2.z) + (layerMasks.w * normal3.z);
    output.specular.z = roughnessMasked;

    float maskedSpecular = (layerMasks.x * invSpecularIntensityLayer0) + (layerMasks.y * invSpecularIntensityLayer1) + (layerMasks.z * invSpecularIntensityLayer2) + (layerMasks.w * invSpecularIntensityLayer3);
    output.specular.w = maskedSpecular;
    output.specular.xy = specularAdjust.xy * float2(0.00392156886, 0.00392156886) + float2(0.00196078443, 0.00196078443);

    float2 normal0Offs = normal0.wy * float2(2, -2) + float2(-1, 1);
    float Z0 = sqrt(1 + -min(1, dot(normal0Offs.xy, normal0Offs.xy)));
    float3 normal0Unpacked = float3(normal0Offs.x, normal0Offs.y, Z0);

    float2 normal1Offs = normal1.wy * float2(2, -2) + float2(-1, 1);
    float Z1 = sqrt(1 + -min(1, dot(normal1Offs.xy, normal1Offs.xy)));
    float3 normal1Unpacked = float3(normal1Offs.x, normal1Offs.y, Z1);

    float2 normal2Offs = normal2.wy * float2(2, -2) + float2(-1, 1);
    float Z2 = sqrt(1 + -min(1, dot(normal2Offs.xy, normal2Offs.xy)));
    float3 normal2Unpacked = float3(normal2Offs.x, normal2Offs.y, Z2);

    float2 normal3Offs = normal3.wy * float2(2, -2) + float2(-1, 1);
    float Z3 = sqrt(1 + -min(1, dot(normal3Offs.xy, normal3Offs.xy)));
    float3 normal3Unpacked = float3(normal3Offs.x, normal3Offs.y, Z3);

    float3 normalBlend = (layerMasks.xxx * normal0Unpacked) + (layerMasks.yyy * normal1Unpacked) + (layerMasks.zzz * normal2Unpacked) + (layerMasks.www * normal3Unpacked);
    float3 normalCombined = (normalBlend.zzz * inNormal.xyz) + (normalBlend.xxx * inTangent.xyz) + (inBiTangent.xyz * normalBlend.yyy);
    float Z = rsqrt(dot(normalCombined, normalCombined));
    normalCombined *= Z.xxx;
    float unkN = (normalCombined.z < 0.0) ? 1.0 : 0.0;
    float unkN2 = dot(abs(normalCombined), float3(1, 1, 1));
    float2 unkN3 = normalCombined.xy / unkN2.xx;
    float2 unkN4 = unkN3.x >= 0 || unkN3.y >= 0 ? float2(1, 1) : float2(0, 0);
    unkN4 = unkN4.x != 0 || unkN4.y != 0 ? float2(1, 1) : float2(-1, -1);
    unkN4 = -unkN4 * abs(unkN3) + unkN4;
    float2 finalNormal = unkN3;
    output.normal.xy = finalNormal * float2(0.5, 0.5) + float2(0.5, 0.5);
    output.normal.w = 1;
    output.unknown.x = v2;
    output.unknown.yzw = float3(0, 0, 0);

    float depth = abs(input.eyeSpacePosition.z / input.eyeSpacePosition.w);
    float2 uv = input.uv;

    if (tEditorParams.brushParams.isEnabled)
    {
        float d = dist(tEditorParams.brushParams.position.xz, input.fragPos.xz) * (1.0 / tEditorParams.brushParams.size);
        float3 brushColor = float3(0.1, 0.1, 0.7);

        if (tEditorParams.brushParams.mode == 0)
        {
            float3 brushValue = lerp(brushColor, float3(0.0, 0.0, 0.0), saturate(d));
            output.diffuse.rgb += brushValue;
        }
        else if (tEditorParams.brushParams.mode == 1)
        {
            float minDist = 0.99;
            float maxDist = minDist + (tEditorParams.brushParams.size * 0.0002);
            if (d > minDist && d < maxDist)
            {
                output.diffuse.rgb += float3(1.0, 1.0, 1.0);
            }
        }
    }

    if (tEditorParams.enableAreaGrid)
    {
        float cthickness = 0.001;
        float2 wposMod = float2(fmod(32768.0 + input.fragPos.x, 512.0), fmod(32768.0 + input.fragPos.z, 512.0)) / 512.0;
        if (wposMod.x < cthickness || wposMod.x > 1 - cthickness || wposMod.y < cthickness || wposMod.y > 1 - cthickness)
        {
            output.diffuse.rgb = lerp(float3(0.3, 0.3, 1.0), output.diffuse.rgb, saturate(depth / 800));
        }
    }

    if (tEditorParams.enableChunkGrid)
    {
        float sthickness = 0.006;
        if (uv.x < sthickness || uv.x > 1 - sthickness || uv.y < sthickness || uv.y > 1 - sthickness)
        {
            output.diffuse.rgb = lerp(saturate(output.diffuse.rgb + 0.3), output.diffuse.rgb, saturate(depth / 800.0));
        }
    }

    return output;
}
