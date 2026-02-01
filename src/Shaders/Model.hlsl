cbuffer ModelCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    float3 lightPos;
    float pad0;
    float3 viewPos;
    float pad1;
    float4 lightColor;
    float4 ambientColor;
    float4 objectColor;
};

Texture2D diffuseMap0 : register(t0);
Texture2D normalMap0 : register(t1);
Texture2D diffuseMap1 : register(t2);
Texture2D normalMap1 : register(t3);
Texture2D diffuseMap2 : register(t4);
Texture2D normalMap2 : register(t5);
Texture2D diffuseMap3 : register(t6);
Texture2D normalMap3 : register(t7);

SamplerState samplerWrap : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 tangent : TANGENT;
    float2 normal : NORMAL;
    float2 bitangent : BINORMAL;
    int4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
    float4 color0 : COLOR0;
    float4 color1 : COLOR1;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    int unknown : TEXCOORD2;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float4 color0 : COLOR0;
    float4 color1 : COLOR1;
    float2 uv0 : TEXCOORD1;
    float2 uv1 : TEXCOORD2;
    float3 normal : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 bitangent : TEXCOORD5;
};

struct PSOutput
{
    float4 diffuse : SV_TARGET0;
    float4 specular : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 unknown : SV_TARGET3;
};

float3 UnpackNormal(float4 packednormal)
{
    float3 n;
    n.xy = packednormal.wy * 2 - 1;
    n.z = sqrt(1 - n.x * n.x - n.y * n.y);
    return n;
}

PSInput VSMain(VSInput input)
{
    PSInput output;

    float nX = (input.normal.x - 127.0) / 127.0;
    float nY = (input.normal.y - 127.0) / 127.0;
    float nZ = sqrt((nX * nX) + (nY * nY));
    float3 unpackedNormal = float3(nX, nY, 1.0 - nZ);
    output.normal = unpackedNormal;

    float tX = (input.tangent.x - 127.0) / 127.0;
    float tY = (input.tangent.y - 127.0) / 127.0;
    float tZ = sqrt((tX * tX) + (tY * tY));
    float3 unpackedTangent = float3(tX, tY, 1.0 - tZ);
    output.tangent = unpackedTangent;

    float btX = (input.bitangent.x - 127.0) / 127.0;
    float btY = (input.bitangent.y - 127.0) / 127.0;
    float btZ = sqrt((btX * btX) + (btY * btY));
    float3 unpackedBiTangent = float3(btX, btY, 1.0 - btZ);
    output.bitangent = unpackedBiTangent;

    output.fragPos = mul(float4(input.position, 1.0), model).xyz;

    output.color0 = input.color0;
    output.color1 = input.color1;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;

    output.position = mul(mul(mul(float4(input.position, 1.0), model), view), projection);

    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    float4 color0 = diffuseMap0.Sample(samplerWrap, input.uv0);
    float4 color1 = diffuseMap1.Sample(samplerWrap, input.uv0);
    float4 color2 = diffuseMap2.Sample(samplerWrap, input.uv0);
    float4 color3 = diffuseMap3.Sample(samplerWrap, input.uv0);

    float4 normal0 = normalMap0.Sample(samplerWrap, input.uv0);
    float4 normal1 = normalMap1.Sample(samplerWrap, input.uv0);
    float4 normal2 = normalMap2.Sample(samplerWrap, input.uv0);
    float4 normal3 = normalMap3.Sample(samplerWrap, input.uv0);

    float4 colorData = (color0 * input.color1.b) + (color1 * input.color1.g) + (color2 * input.color1.r) + (color3 * input.color1.a);
    float4 normalData = (normal0 * input.color1.b) + (normal1 * input.color1.g) + (normal2 * input.color1.r) + (normal3 * input.color1.a);

    float3 normal = UnpackNormal(normalData);

    float3 color = colorData.rgb;

    output.diffuse = colorData;
    output.specular = float4(0, 0, 0, 1);
    output.normal = float4(normal.xyz, 1.0);
    output.unknown = float4(0, 0, 0, 0);

    return output;
}
