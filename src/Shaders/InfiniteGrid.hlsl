cbuffer InfiniteGridCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 lineColor;
    float3 viewPos;
    float pad0;
};

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.fragPos = mul(float4(input.position, 1.0), model).xyz;
    output.position = mul(mul(mul(float4(input.position, 1.0), model), view), projection);

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1, 1, 1, 1);
}
