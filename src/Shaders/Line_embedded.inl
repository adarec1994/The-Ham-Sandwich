const char* LineShaderSource = R"(
cbuffer LineCB : register(b0) {
    row_major matrix View;
    row_major matrix Projection;
};

struct VSInput {
    float3 Position : POSITION;
    float4 Color : COLOR;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    output.Position = mul(Projection, mul(View, float4(input.Position, 1.0)));
    output.Color = input.Color;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.Color;
}
)";
