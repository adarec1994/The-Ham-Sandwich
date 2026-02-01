const char* MapTileShaderSource = R"(
cbuffer MapTileCB : register(b0) {
    row_major matrix World;
    row_major matrix View;
    row_major matrix Projection;
    float4 TintColor;
    float Opacity;
    float3 Padding;
};

Texture2D TileTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput {
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(World, float4(input.Position, 1.0));
    output.Position = mul(Projection, mul(View, worldPos));
    output.TexCoord = input.TexCoord;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 texColor = TileTexture.Sample(LinearSampler, input.TexCoord);
    float4 finalColor = texColor * TintColor;
    finalColor.a *= Opacity;
    return finalColor;
}
)";
