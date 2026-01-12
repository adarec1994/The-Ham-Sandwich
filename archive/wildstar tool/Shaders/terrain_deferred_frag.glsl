#version 420 core

out vec4 outDiffuse;
out vec4 outSpecular;
out vec4 outNormal;
out vec4 outUnknown;

in VS_OUT 
{
    vec3 FragPos;
    vec4 Normal;
	float v2;
    vec2 UV;
    vec4 EyeSpacePosition;
	vec4 Tangent;
	vec4 BiTangent;
} fs_in;

struct BrushParameters
{
    int mode;
    vec3 position;
    float size;

    bool isEnabled;
};

struct TerrainEditorParameters
{
    bool enableAreaGrid;
    bool enableChunkGrid;
    BrushParameters brushParams;
};


struct TerrainParameters
{
    vec4 heightScale;
    vec4 heightOffset;
    vec4 parallaxScale;
    vec4 parallaxOffset;
    vec4 metersPerTextureTile;// = vec4(32.0, 32.0, 32.0, 32.0);
    float specularPower;
    vec2 scrollSpeed;
    bool enableColorMap;
    bool enableUnkMap2;
};

layout(binding=0) uniform sampler2D textureLayer0;
layout(binding=1) uniform sampler2D textureLayer1;
layout(binding=2) uniform sampler2D textureLayer2;
layout(binding=3) uniform sampler2D textureLayer3;
layout(binding=4) uniform sampler2D textureNormal0;
layout(binding=5) uniform sampler2D textureNormal1;
layout(binding=6) uniform sampler2D textureNormal2;
layout(binding=7) uniform sampler2D textureNormal3;
layout(binding=8) uniform sampler2D textureBlendMap0;
layout(binding=9) uniform sampler2D textureColorMap;
layout(binding=10) uniform sampler2D textureBlendMap1;

uniform TerrainParameters terrainParams;
uniform TerrainEditorParameters tEditorParams;

#define saturate(value) clamp(value,0.0,1.0)
#define rsqrt(x) 1 / sqrt(x)
#define dist(p0, pf) sqrt((pf.x-p0.x)*(pf.x-p0.x)+(pf.y-p0.y)*(pf.y-p0.y))

void main()
{
	vec3 inPosition = fs_in.FragPos;
	vec4 inNormal = fs_in.Normal;
	float v2 = fs_in.v2;
	vec2 inUV = fs_in.UV;
	vec4 inTangent = fs_in.Tangent;
	vec4 inBiTangent = fs_in.BiTangent;

	vec4 specularAdjust = vec4(2.0, 3.0, 0.0, 0.0);		// cb0[2]; xy value is stored in outSpecular.xy (found just black)
	vec4 colorMapAdjustment = vec4(2.0, 1.0, 0.0, 0.0);	// cb0[4]; vec4(Multiply RGB, Multiply A, Add RGB, Add A)

	// Meters per texture tile per layer (db value / 32.0) only z component is used
	vec4 metersPerTexA = vec4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.x, 0.0);	// cb1[0];
	vec4 metersPerTexB = vec4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.y, 0.0);	// cb1[1];
	vec4 metersPerTexC = vec4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.z, 0.0);	// cb1[2];
	vec4 metersPerTexD = vec4(0.0, 0.0, 32.0 / terrainParams.metersPerTextureTile.w, 0.0);	// cb1[3];

	// These seem to adjust specular level per layer (xyz color, w intensity)
	vec4 layer0SpecularAdjust = vec4(0, 0, 0, 0);	// cb1[4];
	vec4 layer1SpecularAdjust = vec4(0, 0, 0, 0);	// cb1[5];
	vec4 layer2SpecularAdjust = vec4(0, 0, 0, 0);	// cb1[6];
	vec4 layer3SpecularAdjust = vec4(0, 0, 0, 0);	// cb1[7];

	vec4 heightScale = vec4(terrainParams.heightScale[0], terrainParams.heightScale[1], terrainParams.heightScale[2], terrainParams.heightScale[3]);	// cb1[8];
	vec4 heightOffset = vec4(terrainParams.heightOffset[0], terrainParams.heightOffset[1], terrainParams.heightOffset[2], terrainParams.heightOffset[3]);	// cb1[9];

	vec4 parallaxScale = vec4(
		terrainParams.parallaxScale[0] / 100.0,
		terrainParams.parallaxScale[1] / 100.0,
		terrainParams.parallaxScale[2] / 100.0,
		terrainParams.parallaxScale[3] / 100.0
	);	// cb1[10];		// tbl value divided by 100.0
	vec4 parallaxOffset = vec4(
		-terrainParams.parallaxOffset[0] / 100.0,
		-terrainParams.parallaxOffset[1] / 100.0,
		-terrainParams.parallaxOffset[2] / 100.0,
		-terrainParams.parallaxOffset[3] / 100.0
	);	// cb1[11];	// tbl value divided by 100.0 and inverted sign (-+)

	// Not used in this shader variant :(
	//vec4 cb1_12 = cb1[12];
	//vec4 cb1_13 = cb1[13];
	//vec4 cb1_14 = cb1[14];
	//vec4 cb1_15 = cb1[15];

	// Not used in this shader variant :(
	//vec4 cb1_16 = cb1[16];
	//vec4 cb1_17 = cb1[17];

	// These inUV scale offsets are calculated based on subchunk coordinates
	// They don't matter for me since I don't merge all the textures for batching
	// The game can do that on its own, not worth the computations
	//vec4 blendMap0inUVScaleOffset = cb1[18];
	//vec4 blendMap1inUVScaleOffset = cb1[19];
	//vec4 colorMapinUVScaleOffset = cb1[20];

	vec2 colorMapinUV = inUV.xy;// * colorMapinUVScaleOffset.xy + colorMapinUVScaleOffset.zw;
	vec4 colorMap = texture(textureColorMap, colorMapinUV).xyzw;
	vec4 adjustedColorMap = colorMap * colorMapAdjustment.xxxy + colorMapAdjustment.zzzw;

	vec2 layer0inUV = inUV.xy * metersPerTexA.zz + metersPerTexA.xy;
	vec4 layer0 = texture(textureLayer0, layer0inUV).xyzw;
	vec4 normal0 = texture(textureNormal0, layer0inUV).xyzw;
	float specularIntensityLayer0 = layer0SpecularAdjust.w * layer0.w;
	vec3 specularValueLayer0 = layer0.xyz * specularIntensityLayer0.xxx;
	vec3 diffuseLayer0 = -layer0.xyz * specularIntensityLayer0.xxx + layer0.xyz;
	float invSpecularIntensityLayer0 = -layer0.w * layer0SpecularAdjust.w + layer0.w;
	vec3 specularColorLayer0 = layer0SpecularAdjust.xyz * specularValueLayer0.xyz;

	vec2 layer1inUV = inUV.xy * metersPerTexB.zz + metersPerTexB.xy;
	vec4 layer1 = texture(textureLayer1, layer1inUV).xyzw;
	vec4 normal1 = texture(textureNormal1, layer1inUV).xyzw;
	float specularIntensityLayer1 = layer1SpecularAdjust.w * layer1.w;
	vec3 specularValueLayer1 = layer1.xyz * specularIntensityLayer1.xxx;
	vec3 diffuseLayer1 = -layer1.xyz * specularIntensityLayer1.xxx + layer1.xyz;
	float invSpecularIntensityLayer1 = -layer1.w * layer1SpecularAdjust.w + layer1.w;
	vec3 specularColorLayer1 = layer1SpecularAdjust.xyz * specularValueLayer1.xyz;

	vec2 layer2inUV = inUV.xy * metersPerTexC.zz + metersPerTexC.xy;
	vec4 layer2 = texture(textureLayer2, layer2inUV).xyzw;
	vec4 normal2 = texture(textureNormal2, layer2inUV).xyzw;
	float specularIntensityLayer2 = layer2SpecularAdjust.w * layer2.w;
	vec3 specularValueLayer2 = specularIntensityLayer2.xxx * layer2.xyz;
	vec3 diffuseLayer2 = -layer2.xyz * specularIntensityLayer2.xxx + layer2.xyz;
	float invSpecularIntensityLayer2 = -layer2.w * layer2SpecularAdjust.w + layer2.w;
	vec3 specularColorLayer2 = layer2SpecularAdjust.xyz * specularValueLayer2;

	vec2 layer3inUV = inUV.xy * metersPerTexD.zz + metersPerTexD.xy;
	vec4 normal3 = texture(textureNormal3, layer3inUV).xyzw;
	vec4 layer3 = texture(textureLayer3, layer3inUV).xyzw;
	float specularIntensityLayer3 = layer3SpecularAdjust.w * layer3.w;
	vec3 specularValueLayer3 = layer3.xyz * specularIntensityLayer3.xxx;
	vec3 diffuseLayer3 = -layer3.xyz * specularIntensityLayer3.xxx + layer3.xyz;
	float invSpecularIntensityLayer3 = -layer3.w * layer3SpecularAdjust.w + layer3.w;
	vec3 specularColorLayer3 = layer3SpecularAdjust.xyz * specularValueLayer3;

	vec2 blendMap1inUV = inUV.xy;// * blendMap1inUVScaleOffset.xy + blendMap1inUVScaleOffset.zw;
	vec4 blendMap1 = texture(textureBlendMap1, blendMap1inUV).xyzw;
	vec3 scaledBlendMap1 = blendMap1.xyz * vec3(2.0666666,2.03225803,2.0666666) + vec3(-1,-1,-1);

	vec2 blendMap0inUV = inUV.xy;// * blendMap0inUVScaleOffset.xy + blendMap0inUVScaleOffset.zw;
	vec4 blendMap0 = texture(textureBlendMap0, blendMap0inUV).xyzw;
	vec4 blendMapMixed;
	blendMapMixed.xyz = blendMap0.xyz + scaledBlendMap1;
	float blendMapDot = dot(blendMapMixed.xyz, vec3(1,1,1));
	blendMapMixed.w = 1 + -blendMapDot;
	blendMapMixed = saturate(blendMapMixed.xyzw);
	blendMapMixed *= blendMap0.wwww;
	blendMapDot = dot(blendMapMixed, vec4(1,1,1,1));

	vec4 layerHeightMaps = vec4(normal0.x, normal1.x, normal2.x, normal3.x);
	vec4 layerHeightMapsScaled = layerHeightMaps * heightScale.xyzw + heightOffset.xyzw;
	vec4 layerHeightMapsBlended = blendMapMixed * layerHeightMapsScaled + vec4(0.00100000005,0.00100000005,0.00100000005,0.00100000005);
	vec4 layerHeightMapsBlendPow = layerHeightMapsBlended * layerHeightMapsBlended;
	float heightMapsDot = dot(layerHeightMapsBlended, layerHeightMapsBlended);
	vec4 heightXblend = layerHeightMapsBlendPow * blendMapDot.xxxx;
	float blendMapDotInv = 1 + -blendMapDot;
	vec4 layerMasks = heightXblend / heightMapsDot.xxxx;

	vec3 specColorCombined = (layerMasks.xxx * specularColorLayer0) + (layerMasks.yyy * specularColorLayer1) + (layerMasks.zzz * specularColorLayer2) + (layerMasks.www * specularColorLayer3);
	vec3 specularColor = specColorCombined * adjustedColorMap.xyz;

	vec3 diffuseCombined = (layerMasks.xxx * diffuseLayer0) + (layerMasks.yyy * diffuseLayer1.xyz) + (layerMasks.zzz * diffuseLayer2) + (layerMasks.www * diffuseLayer3);
	diffuseCombined += blendMapDotInv.xxx * vec3(0.5,0.5,0.5);
	vec3 diffuse = diffuseCombined * adjustedColorMap.xyz + specularColor;

	outNormal.z = adjustedColorMap.w;		// Painted shadow map (most likely in color map texture alpha channel)

	float specColorDot = dot(specularColor, vec3(0.212500006,0.715399981,0.0720999986));
	float diffuseDot = dot(diffuse, vec3(0.212500006,0.715399981,0.0720999986));

	outDiffuse.xyz = diffuse;

	float scaledDiffuseDot = 0.00100000005 + diffuseDot;
	outDiffuse.w = specColorDot / scaledDiffuseDot;

	float roughnessMasked = (layerMasks.x * normal0.z) + (layerMasks.y * normal1.z) + (layerMasks.z * normal2.z) + (layerMasks.w * normal3.z);
	outSpecular.z = roughnessMasked;	// Or I think it's roughness, doesn't matter just gonna output the correct thing

	float maskedSpecular = (layerMasks.x * invSpecularIntensityLayer0) + (layerMasks.y * invSpecularIntensityLayer1) + (layerMasks.z * invSpecularIntensityLayer2) + (layerMasks.w * invSpecularIntensityLayer3);
	outSpecular.w = maskedSpecular;
	outSpecular.xy = specularAdjust.xy * vec2(0.00392156886,0.00392156886) + vec2(0.00196078443,0.00196078443);

	vec2 normal0Offs = normal0.wy * vec2(2,-2) + vec2(-1,1);
	float Z0 = sqrt(1 + -min(1, dot(normal0Offs.xy, normal0Offs.xy)));
	vec3 normal0Unpacked = vec3(normal0Offs.x, normal0Offs.y, Z0);

	vec2 normal1Offs = normal1.wy * vec2(2,-2) + vec2(-1,1);
	float Z1 = sqrt(1 + -min(1, dot(normal1Offs.xy, normal1Offs.xy)));
	vec3 normal1Unpacked = vec3(normal1Offs.x, normal1Offs.y, Z1);

	vec2 normal2Offs = normal2.wy * vec2(2,-2) + vec2(-1,1);
	float Z2 = sqrt(1 + -min(1, dot(normal2Offs.xy, normal2Offs.xy)));
	vec3 normal2Unpacked = vec3(normal2Offs.x, normal2Offs.y, Z2);

	vec2 normal3Offs = normal3.wy * vec2(2,-2) + vec2(-1,1);
	float Z3 = sqrt(1 + -min(1, dot(normal3Offs.xy, normal3Offs.xy)));
	vec3 normal3Unpacked = vec3(normal3Offs.x, normal3Offs.y, Z3);

	vec3 normalBlend = (layerMasks.xxx * normal0Unpacked) + (layerMasks.yyy * normal1Unpacked) + (layerMasks.zzz * normal2Unpacked) + (layerMasks.www * normal3Unpacked);
	vec3 normalCombined = (normalBlend.zzz * inNormal.xyz) + (normalBlend.xxx * inTangent.xyz) + (inBiTangent.xyz * normalBlend.yyy);
	//normalCombined += inNormal.xyz * blendMapDotInv.xxx;
	float Z = rsqrt(dot(normalCombined, normalCombined));
	normalCombined *= Z.xxx;
	float unkN = (normalCombined.z < 0.0) ? 1.0 : 0.0; //cmp(normalCombined.z < 0);
	float unkN2 = dot(abs(normalCombined), vec3(1,1,1));
	vec2 unkN3 = normalCombined.xy / unkN2.xx;
	vec2 unkN4 = unkN3.x >= 0 || unkN3.y >= 0 ? vec2(1,1) : vec2(0,0);//cmp(unkN3 >= vec2(0,0));
	unkN4 = unkN4 != vec2(0, 0) ? vec2(1,1) : vec2(-1,-1);
	unkN4 = -unkN4 * abs(unkN3) + unkN4;
	vec2 normal = unkN3;//unkN.xx != vec2(0, 0) ? unkN4 : unkN3;
	outNormal.xy = normal * vec2(0.5, 0.5) + vec2(0.5, 0.5);
	outNormal.w = 1;
	outUnknown.x = v2.x; //  -position.z; in vertex shader
	outUnknown.yzw = vec3(0,0,0);

	// Editor only code
	float depth = abs(fs_in.EyeSpacePosition.z / fs_in.EyeSpacePosition.w);
	vec2 uv = fs_in.UV;

    if (tEditorParams.brushParams.isEnabled)
    {
        float d = dist(tEditorParams.brushParams.position.xz, fs_in.FragPos.xz) * (1.0 / tEditorParams.brushParams.size);
        vec3 brushColor = vec3(0.1, 0.1, 0.7);

        if (tEditorParams.brushParams.mode == 0)
        {
            vec3 brushValue = mix(brushColor, vec3(0.0, 0.0, 0.0), clamp(d, 0.0, 1.0));
            outDiffuse.rgb += brushValue;
        }
        else if (tEditorParams.brushParams.mode == 1)
        {
            float minDist = 0.99;
            float maxDist = minDist + (tEditorParams.brushParams.size * 0.0002);//0.999;
            if (d > minDist && d < maxDist)
            {
                outDiffuse.rgb += 1.0.xxx;
                //outDiffuse = clamp(outDiffuse, 0.0, 1.0);
            }
        }
    }

    // Draw Chunk edges
    if(tEditorParams.enableAreaGrid)
    {
        float cthickness = 0.001;
        vec2 wposMod = vec2(mod(32768.0 + fs_in.FragPos.x, 512.0), mod(32768.0 + fs_in.FragPos.z, 512.0)) / 512.0;
        if (wposMod.x < cthickness || wposMod.x > 1 - cthickness || wposMod.y < cthickness || wposMod.y > 1 - cthickness)
        {
            outDiffuse.rgb = mix(vec3(0.3, 0.3, 1.0), outDiffuse.rgb, clamp(depth / 800, 0, 1));
        }
    }

    // Draw SubChunk edges
    if(tEditorParams.enableChunkGrid)
    {
        float sthickness = 0.006;
        if (uv.x < sthickness || uv.x > 1 - sthickness || uv.y < sthickness || uv.y > 1 - sthickness)
        {
            outDiffuse.rgb = mix(clamp(outDiffuse.rgb + 0.3, 0, 1), outDiffuse.rgb, clamp(depth / 800.0, 0, 1));
        }
    }

	//outDiffuse.rgb += blendMap0.yyy * 0.4;
	return;
}