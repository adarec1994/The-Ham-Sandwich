cbuffer IconCB : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float3 vertexPosition_worldspace;
    float pad0;
    float3 cameraPos;
    float viewportAspect;
};

Texture2D u_texture : register(t0);
SamplerState samplerClamp : register(s0);

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float dist : TEXCOORD1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    float size = 0.04;
    float4x4 VP = mul(view, projection);

    float4 pos = mul(float4(vertexPosition_worldspace, 1.0), VP);
    pos /= pos.w;
    pos.xy += input.position.xy * float2(size, size * viewportAspect);
    output.position = pos;

    output.dist = length(vertexPosition_worldspace - cameraPos);
    output.uv = input.position.xy + float2(0.5, 0.5);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    uv.y = 1.0 - uv.y;
    float4 color = u_texture.Sample(samplerClamp, uv);
    color.a *= 1.0 - smoothstep(0.0, 500.0, input.dist);
    return color;
}
