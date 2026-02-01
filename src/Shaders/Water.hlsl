cbuffer WaterCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 bitangent : BINORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float unk0 : TEXCOORD1;
    int unk1 : BLENDINDICES0;
    float4 blendMask : COLOR1;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 pos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 tangent : TEXCOORD2;
    float4 bitangent : TEXCOORD3;
    float2 uv : TEXCOORD4;
    float4 color : COLOR0;
    float unk0 : TEXCOORD5;
    int unk1 : BLENDINDICES0;
    float4 blendMask : COLOR1;
};

float3 UnpackNormal(float4 packednormal)
{
    float3 normal;
    normal.xy = packednormal.wy * 2 - 1;
    normal.z = sqrt(1 - normal.x * normal.x - normal.y * normal.y);
    return normal;
}

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.pos = input.position;
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;
    output.uv = input.uv;
    output.color = input.color;
    output.unk0 = input.unk0;
    output.unk1 = input.unk1;
    output.blendMask = input.blendMask;

    output.position = mul(mul(mul(float4(input.position, 1.0), model), view), projection);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.color.rgb, 1.0);
}
