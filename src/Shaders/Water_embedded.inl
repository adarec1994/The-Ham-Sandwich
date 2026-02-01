const char* WaterShaderSource = R"(
cbuffer WaterCB : register(b0) {
    row_major matrix World;
    row_major matrix View;
    row_major matrix Projection;
    float4 CameraPosition;
    float4 WaterColor;
    float Time;
    float3 padding;
};

struct VSInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    float Depth : TEXCOORD1;
    int Type : TEXCOORD2;
    float4 Color2 : COLOR1;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float4 Color : COLOR0;
    float Depth : TEXCOORD3;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(World, float4(input.Position, 1.0));
    output.WorldPos = worldPos.xyz;
    output.Position = mul(Projection, mul(View, worldPos));
    
    output.Normal = normalize(mul((float3x3)World, input.Normal));
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    output.Depth = input.Depth;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = input.Color;
    color.a = 0.7;
    return color;
}
)";
