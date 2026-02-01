cbuffer FontCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 projection;
    float4 textColor;
};

Texture2D u_texture : register(t0);
SamplerState samplerClamp : register(s0);

struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.uv = input.uv;
    output.position = mul(mul(float4(input.position, 0.0, 1.0), model), projection);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    float text = u_texture.Sample(samplerClamp, uv).r;
    return float4(textColor.rgb, text * textColor.a);
}
