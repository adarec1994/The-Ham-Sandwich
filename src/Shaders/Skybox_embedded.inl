const char* SkyboxShaderSource = R"(
cbuffer SkyboxCB : register(b0) {
    row_major matrix View;
    row_major matrix Projection;
    float4 TopColor;
    float4 BottomColor;
};

TextureCube SkyboxTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput {
    float3 Position : POSITION;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    matrix viewNoTranslation = View;
    viewNoTranslation[3][0] = 0;
    viewNoTranslation[3][1] = 0;
    viewNoTranslation[3][2] = 0;
    
    float4 pos = mul(Projection, mul(viewNoTranslation, float4(input.Position, 1.0)));
    output.Position = pos.xyww;
    output.TexCoord = input.Position;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float t = saturate(input.TexCoord.y * 0.5 + 0.5);
    float4 gradient = lerp(BottomColor, TopColor, t);
    
    float4 texColor = SkyboxTexture.Sample(LinearSampler, input.TexCoord);
    
    float4 finalColor = texColor.a > 0.01 ? texColor : gradient;
    
    return finalColor;
}
)";
