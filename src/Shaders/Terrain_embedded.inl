const char* TerrainShaderSource = R"(
cbuffer TerrainCB : register(b0) {
    row_major matrix Model;
    row_major matrix View;
    row_major matrix Projection;
    float4 TexScale;
    float4 BaseColor;
    float4 HighlightColor;
    float3 CamPosition;
    int HasColorMap;
    float4 terrainParams[5];
    float4 terrainParams2;
    float4 editorParams[3];
    float4 envParams[8];
};

Texture2D BlendMap : register(t0);
Texture2D ColorMap : register(t1);
Texture2D Layer0Diffuse : register(t2);
Texture2D Layer1Diffuse : register(t3);
Texture2D Layer2Diffuse : register(t4);
Texture2D Layer3Diffuse : register(t5);
Texture2D Layer0Normal : register(t6);
Texture2D Layer1Normal : register(t7);
Texture2D Layer2Normal : register(t8);
Texture2D Layer3Normal : register(t9);

SamplerState LinearSampler : register(s0);

struct VSInput {
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    float3 Tangent : TEXCOORD3;
    float3 Bitangent : TEXCOORD4;
};

float3 UnpackNormal(float4 packedNormal) {
    float3 n;
    n.xy = packedNormal.wy * 2.0 - 1.0;
    n.z = sqrt(max(0.0, 1.0 - n.x * n.x - n.y * n.y));
    return n;
}

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(Model, float4(input.Position, 1.0));
    output.WorldPos = worldPos.xyz;
    output.Position = mul(Projection, mul(View, worldPos));
    
    output.Normal = normalize(mul((float3x3)Model, input.Normal));
    output.Tangent = normalize(mul((float3x3)Model, input.Tangent.xyz));
    output.Bitangent = cross(output.Normal, output.Tangent) * input.Tangent.w;
    
    output.TexCoord = input.TexCoord;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 blend = BlendMap.Sample(LinearSampler, input.TexCoord);
    
    float2 uv0 = input.TexCoord * TexScale.x;
    float2 uv1 = input.TexCoord * TexScale.y;
    float2 uv2 = input.TexCoord * TexScale.z;
    float2 uv3 = input.TexCoord * TexScale.w;
    
    float4 diffuse0 = Layer0Diffuse.Sample(LinearSampler, uv0);
    float4 diffuse1 = Layer1Diffuse.Sample(LinearSampler, uv1);
    float4 diffuse2 = Layer2Diffuse.Sample(LinearSampler, uv2);
    float4 diffuse3 = Layer3Diffuse.Sample(LinearSampler, uv3);
    
    float4 diffuse = diffuse0 * blend.r + diffuse1 * blend.g + diffuse2 * blend.b + diffuse3 * blend.a;
    
    if (HasColorMap != 0) {
        float4 colormap = ColorMap.Sample(LinearSampler, input.TexCoord);
        diffuse.rgb *= colormap.rgb * 2.0;
    }
    
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float NdotL = max(dot(normalize(input.Normal), lightDir), 0.0);
    float3 lighting = float3(0.3, 0.3, 0.3) + float3(1.0, 0.95, 0.9) * NdotL;
    
    float3 finalColor = diffuse.rgb * lighting * BaseColor.rgb;
    
    if (HighlightColor.a > 0.0) {
        finalColor = lerp(finalColor, HighlightColor.rgb, HighlightColor.a);
    }
    
    return float4(finalColor, 1.0);
}
)";
