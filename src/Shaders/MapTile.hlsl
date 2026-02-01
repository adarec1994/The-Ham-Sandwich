cbuffer MapTileCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    float3 viewPos;
    float pad0;
};

Texture2D u_texture : register(t0);
SamplerState samplerClamp : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.fragPos = mul(float4(input.position, 1.0), model).xyz;
    output.uv = input.uv;
    output.position = mul(mul(mul(float4(input.position, 1.0), model), view), projection);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    return u_texture.Sample(samplerClamp, uv);
}
