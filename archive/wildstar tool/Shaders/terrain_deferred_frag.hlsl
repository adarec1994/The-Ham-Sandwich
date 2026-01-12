// ---- Created with 3Dmigoto v1.3.16 on Sun Nov 20 00:16:27 2022
Texture2D<float4> textureColorMap : register(t10);

Texture2D<float4> textureBlendMap1 : register(t9);

Texture2D<float4> textureBlendMap0 : register(t8);

Texture2D<float4> textureNormal3 : register(t7);

Texture2D<float4> textureNormal2 : register(t6);

Texture2D<float4> textureNormal1 : register(t5);

Texture2D<float4> textureNormal0 : register(t4);

Texture2D<float4> textureLayer3 : register(t3);

Texture2D<float4> textureLayer2 : register(t2);

Texture2D<float4> textureLayer1 : register(t1);

Texture2D<float4> textureLayer0 : register(t0);

SamplerState samplerStateColorMap : register(s10);

SamplerState samplerStateBlendMap1 : register(s9);

SamplerState samplerStateBlendMap0 : register(s8);

SamplerState samplerStateNormal3 : register(s7);

SamplerState samplerStateNormal2 : register(s6);

SamplerState samplerStateNormal1 : register(s5);

SamplerState samplerStateNormal0 : register(s4);

SamplerState samplerStateLayer3 : register(s3);

SamplerState samplerStateLayer2 : register(s2);

SamplerState samplerStateLayer1 : register(s1);

SamplerState samplerStateLayer0 : register(s0);

cbuffer cb1 : register(b1)
{
  float4 cb1[21];
}

cbuffer cb0 : register(b0)
{
  float4 cb0[5];
}

// 3Dmigoto declarations
#define cmp -


void main(
	float4 inPosition : SV_Position0,
	float4 inNormal : TEXCOORD0,	// y
	float4 v2 : TEXCOORD1,
	float4 inUV : TEXCOORD2,
	float4 inTangent : TEXCOORD3,		// x
	float4 inBiTangent : TEXCOORD4,		// z
	float3 v6 : TEXCOORD5,
	out float4 outDiffuse : SV_Target0,
	out float4 outSpecular : SV_Target1,
	out float4 outNormal : SV_Target2,
	out float4 outUnknown : SV_Target3)
{
	uint4 bitmask, uiDest;
	float4 fDest;

	float4 specularAdjust = cb0[2];		// xy value is stored in outSpecular.xy (found just black)
	float4 colorMapAdjustment = cb0[4];	// float4(Multiply RGB, Multiply A, Add RGB, Add A)

	// Meters per texture tile per layer (db value / 32.0) only z component is used
	float4 metersPerTexA = cb1[0];
	float4 metersPerTexB = cb1[1];
	float4 metersPerTexC = cb1[2];
	float4 metersPerTexD = cb1[3];

	// These seem to adjust specular level per layer (xyz color, w intensity)
	float4 layer0SpecularAdjust = cb1[4];
	float4 layer1SpecularAdjust = cb1[5];
	float4 layer2SpecularAdjust = cb1[6];
	float4 layer3SpecularAdjust = cb1[7];
	//layer1SpecularAdjust = float4(3,3,3,10);

	float4 heightScale = cb1[8];
	float4 heightOffset = cb1[9];

	float4 parallaxScale = cb1[10];		// tbl value divided by 100.0
	float4 parallaxOffset = cb1[11];	// tbl value divided by 100.0 and inverted sign (-+)

	// Not used in this shader variant :(
	float4 cb1_12 = cb1[12];
	float4 cb1_13 = cb1[13];
	float4 cb1_14 = cb1[14];
	float4 cb1_15 = cb1[15];

	// Not used in this shader variant :(
	float4 cb1_16 = cb1[16];
	float4 cb1_17 = cb1[17];

	// These inUV scale offsets are calculated based on subchunk coordinates
	// They don't matter for me since I don't merge all the textures for batching
	// The game can do that on its own, not worth the computations
	float4 blendMap0inUVScaleOffset = cb1[18];
	float4 blendMap1inUVScaleOffset = cb1[19];
	float4 colorMapinUVScaleOffset = cb1[20];

	float2 colorMapinUV = inUV.xy * colorMapinUVScaleOffset.xy + colorMapinUVScaleOffset.zw;
	float4 colorMap = textureColorMap.Sample(samplerStateColorMap, colorMapinUV).xyzw;
	float4 adjustedColorMap = colorMap * colorMapAdjustment.xxxy + colorMapAdjustment.zzzw;

	float2 layer0inUV = inUV.xy * metersPerTexA.zz + metersPerTexA.xy;
	float4 layer0 = textureLayer0.Sample(samplerStateLayer0, layer0inUV).xyzw;
	float4 normal0 = textureNormal0.Sample(samplerStateNormal0, layer0inUV).xyzw;
	float specularIntensityLayer0 = layer0SpecularAdjust.w * layer0.w;
	float3 specularValueLayer0 = layer0.xyz * specularIntensityLayer0.xxx;
	float3 diffuseLayer0 = -layer0.xyz * specularIntensityLayer0.xxx + layer0.xyz;
	float invSpecularIntensityLayer0 = -layer0.w * layer0SpecularAdjust.w + layer0.w;
	float3 specularColorLayer0 = layer0SpecularAdjust.xyz * specularValueLayer0.xyz;

	float2 layer1inUV = inUV.xy * metersPerTexB.zz + metersPerTexB.xy;
	float4 layer1 = textureLayer1.Sample(samplerStateLayer1, layer1inUV).xyzw;
	float4 normal1 = textureNormal1.Sample(samplerStateNormal1, layer1inUV).xyzw;
	float specularIntensityLayer1 = layer1SpecularAdjust.w * layer1.w;
	float3 specularValueLayer1 = layer1.xyz * specularIntensityLayer1.xxx;
	float3 diffuseLayer1 = -layer1.xyz * specularIntensityLayer1.xxx + layer1.xyz;
	float invSpecularIntensityLayer1 = -layer1.w * layer1SpecularAdjust.w + layer1.w;
	float3 specularColorLayer1 = layer1SpecularAdjust.xyz * specularValueLayer1.xyz;

	float2 layer2inUV = inUV.xy * metersPerTexC.zz + metersPerTexC.xy;
	float4 layer2 = textureLayer2.Sample(samplerStateLayer2, layer2inUV).xyzw;
	float4 normal2 = textureNormal2.Sample(samplerStateNormal2, layer2inUV).xyzw;
	float specularIntensityLayer2 = layer2SpecularAdjust.w * layer2.w;
	float3 specularValueLayer2 = specularIntensityLayer2.xxx * layer2.xyz;
	float3 diffuseLayer2 = -layer2.xyz * specularIntensityLayer2.xxx + layer2.xyz;
	float invSpecularIntensityLayer2 = -layer2.w * layer2SpecularAdjust.w + layer2.w;
	float3 specularColorLayer2 = layer2SpecularAdjust.xyz * specularValueLayer2;

	float2 layer3inUV = inUV.xy * metersPerTexD.zz + metersPerTexD.xy;
	float4 normal3 = textureNormal3.Sample(samplerStateNormal3, layer3inUV).xyzw;
	float4 layer3 = textureLayer3.Sample(samplerStateLayer3, layer3inUV).xyzw;
	float specularIntensityLayer3 = layer3SpecularAdjust.w * layer3.w;
	float3 specularValueLayer3 = layer3.xyz * specularIntensityLayer3.xxx;
	float3 diffuseLayer3 = -layer3.xyz * specularIntensityLayer3.xxx + layer3.xyz;
	float invSpecularIntensityLayer3 = -layer3.w * layer3SpecularAdjust.w + layer3.w;
	float3 specularColorLayer3 = layer3SpecularAdjust.xyz * specularValueLayer3;

	float2 blendMap1inUV = inUV.xy * blendMap1inUVScaleOffset.xy + blendMap1inUVScaleOffset.zw;
	float4 blendMap1 = textureBlendMap1.Sample(samplerStateBlendMap1, blendMap1inUV).xyzw;
	float3 scaledBlendMap1 = blendMap1.xyz * float3(2.0666666,2.03225803,2.0666666) + float3(-1,-1,-1);

	float2 blendMap0inUV = inUV.xy * blendMap0inUVScaleOffset.xy + blendMap0inUVScaleOffset.zw;
	float4 blendMap0 = textureBlendMap0.Sample(samplerStateBlendMap0, blendMap0inUV).xyzw;
	float4 blendMapMixed;
	blendMapMixed.xyz = blendMap0.xyz + scaledBlendMap1;
	float blendMapDot = dot(blendMapMixed.xyz, float3(1,1,1));
	blendMapMixed.w = 1 + -blendMapDot;
	blendMapMixed = saturate(blendMapMixed.xyzw);
	blendMapMixed *= blendMap0.wwww;
	blendMapDot = dot(blendMapMixed, float4(1,1,1,1));

	float4 layerHeightMaps = float4(normal0.x, normal1.x, normal2.x, normal3.x);
	float4 layerHeightMapsScaled = layerHeightMaps * heightScale.xyzw + heightOffset.xyzw;
	float4 layerHeightMapsBlended = blendMapMixed * layerHeightMapsScaled + float4(0.00100000005,0.00100000005,0.00100000005,0.00100000005);
	float4 layerHeightMapsBlendPow = layerHeightMapsBlended * layerHeightMapsBlended;
	float heightMapsDot = dot(layerHeightMapsBlended, layerHeightMapsBlended);
	float4 heightXblend = layerHeightMapsBlendPow * blendMapDot.xxxx;
	float blendMapDotInv = 1 + -blendMapDot;
	float4 layerMasks = heightXblend / heightMapsDot.xxxx;

	float3 specColorCombined = (layerMasks.xxx * specularColorLayer0) + (layerMasks.yyy * specularColorLayer1) + (layerMasks.zzz * specularColorLayer2) + (layerMasks.www * specularColorLayer3);
	float3 specularColor = specColorCombined * adjustedColorMap.xyz;

	float3 diffuseCombined = (layerMasks.xxx * diffuseLayer0) + (layerMasks.yyy * diffuseLayer1.xyz) + (layerMasks.zzz * diffuseLayer2) + (layerMasks.www * diffuseLayer3);
	diffuseCombined += blendMapDotInv.xxx * float3(0.5,0.5,0.5);
	float3 diffuse = diffuseCombined * adjustedColorMap.xyz + specularColor;

	outNormal.z = adjustedColorMap.w;		// Painted shadow map (most likely in color map texture alpha channel)

	float specColorDot = dot(specularColor, float3(0.212500006,0.715399981,0.0720999986));
	float diffuseDot = dot(diffuse, float3(0.212500006,0.715399981,0.0720999986));

	outDiffuse.xyz = diffuse;

	float scaledDiffuseDot = 0.00100000005 + diffuseDot;
	outDiffuse.w = specColorDot / scaledDiffuseDot;

	float roughnessMasked = (layerMasks.x * normal0.z) + (layerMasks.y * normal1.z) + (layerMasks.z * normal2.z) + (layerMasks.w * normal3.z);
	outSpecular.z = roughnessMasked;	// Or I think it's roughness, doesn't matter just gonna output the correct thing

	float maskedSpecular = (layerMasks.x * invSpecularIntensityLayer0) + (layerMasks.y * invSpecularIntensityLayer1) + (layerMasks.z * invSpecularIntensityLayer2) + (layerMasks.w * invSpecularIntensityLayer3);
	outSpecular.w = maskedSpecular;
	outSpecular.xy = specularAdjust.xy * float2(0.00392156886,0.00392156886) + float2(0.00196078443,0.00196078443);

	float2 normal0Offs = normal0.wy * float2(2,-2) + float2(-1,1);
	float Z0 = sqrt(1 + -min(1, dot(normal0Offs.xy, normal0Offs.xy)));
	float3 normal0Unpacked = float3(normal0Offs.x, normal0Offs.y, Z0);

	float2 normal1Offs = normal1.wy * float2(2,-2) + float2(-1,1);
	float Z1 = sqrt(1 + -min(1, dot(normal1Offs.xy, normal1Offs.xy)));
	float3 normal1Unpacked = float3(normal1Offs.x, normal1Offs.y, Z1);

	float2 normal2Offs = normal2.wy * float2(2,-2) + float2(-1,1);
	float Z2 = sqrt(1 + -min(1, dot(normal2Offs.xy, normal2Offs.xy)));
	float3 normal2Unpacked = float3(normal2Offs.x, normal2Offs.y, Z2);

	float2 normal3Offs = normal3.wy * float2(2,-2) + float2(-1,1);
	float Z3 = sqrt(1 + -min(1, dot(normal3Offs.xy, normal3Offs.xy)));
	float3 normal3Unpacked = float3(normal3Offs.x, normal3Offs.y, Z3);

	float3 normalBlend = (layerMasks.xxx * normal0Unpacked) + (layerMasks.yyy * normal1Unpacked) + (layerMasks.zzz * normal2Unpacked) + (layerMasks.www * normal3Unpacked);
	float3 normalCombined = (normalBlend.zzz * inNormal.xyz) + (normalBlend.xxx * inTangent.xyz) + (inBiTangent.xyz * normalBlend.yyy);
	normalCombined += inNormal.xyz * blendMapDotInv.xxx;
	float Z = rsqrt(dot(normalCombined, normalCombined));
	normalCombined *= Z.xxx;
	float unkN = cmp(normalCombined.z < 0);
	float unkN2 = dot(abs(normalCombined), float3(1,1,1));
	float2 unkN3 = normalCombined.xy / unkN2.xx;
	float2 unkN4 = cmp(unkN3 >= float2(0,0));
	unkN4 = unkN4 ? float2(1,1) : float2(-1,-1);
	unkN4 = -unkN4 * abs(unkN3) + unkN4;
	float2 normal = unkN.xx ? unkN4 : unkN3;
	outNormal.xy = normal * float2(0.5,0.5) + float2(0.5,0.5);
	outNormal.w = 1;
	outUnknown.x = v2.x; //  -position.z; in vertex shader
	outUnknown.yzw = float3(0,0,0);
	return;
}