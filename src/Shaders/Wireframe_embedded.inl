const char* WireframeShaderSource = R"(
cbuffer WireframeCB : register(b0) {
    row_major matrix World;
    row_major matrix View;
    row_major matrix Projection;
    float4 Color;
};

struct VSInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float3 Normal : TEXCOORD0;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(World, float4(input.Position, 1.0));
    output.Position = mul(Projection, mul(View, worldPos));
    output.Normal = normalize(mul((float3x3)World, input.Normal));
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return Color;
}
)";
