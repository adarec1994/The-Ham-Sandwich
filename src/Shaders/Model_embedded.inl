const char* ModelShaderSource = R"(
cbuffer ModelCB : register(b0) {
    row_major matrix World;
    row_major matrix View;
    row_major matrix Projection;
    float4 HighlightColor;
    int UseSkinning;
    int UseLayerBlending;
    float2 Padding;
};

cbuffer BoneCB : register(b1) {
    row_major matrix BoneMatrices[200];
};

Texture2D DiffuseMap0 : register(t0);
Texture2D NormalMap0 : register(t1);
Texture2D DiffuseMap1 : register(t2);
Texture2D NormalMap1 : register(t3);
Texture2D DiffuseMap2 : register(t4);
Texture2D NormalMap2 : register(t5);
Texture2D DiffuseMap3 : register(t6);
Texture2D NormalMap3 : register(t7);

SamplerState LinearSampler : register(s0);

struct VSInput {
    float3 Position : POSITION;
    float2 Tangent : TANGENT;
    float2 Normal : NORMAL;
    float2 Bitangent : BINORMAL;
    uint4 BoneIndices : BLENDINDICES;
    float4 BoneWeights : BLENDWEIGHT;
    float4 Color0 : COLOR0;
    float4 Color1 : COLOR1;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

struct PSInput {
    float4 Position : SV_POSITION;
    float3 FragPos : TEXCOORD0;
    float4 Color0 : COLOR0;
    float4 Color1 : COLOR1;
    float2 UV0 : TEXCOORD1;
    float2 UV1 : TEXCOORD2;
    float3 Normal : TEXCOORD3;
    float3 Tangent : TEXCOORD4;
    float3 Bitangent : TEXCOORD5;
};

float3 UnpackNormalByte2(float2 packed) {
    float x = (packed.x * 255.0 - 127.0) / 127.0;
    float y = (packed.y * 255.0 - 127.0) / 127.0;
    float z = sqrt(max(0.0, 1.0 - x * x - y * y));
    return float3(x, y, z);
}

float3 UnpackNormalMap(float4 packedNormal) {
    float3 n;
    n.xy = packedNormal.wy * 2.0 - 1.0;
    n.z = sqrt(max(0.0, 1.0 - n.x * n.x - n.y * n.y));
    return n;
}

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float3 normal = UnpackNormalByte2(input.Normal);
    float3 tangent = UnpackNormalByte2(input.Tangent);
    float3 bitangent = UnpackNormalByte2(input.Bitangent);
    
    output.Normal = normal;
    output.Tangent = tangent;
    output.Bitangent = bitangent;
    
    float4 pos = float4(input.Position, 1.0);
    float3 norm = normal;
    
    if (UseSkinning == 1) {
        matrix skinMatrix = BoneMatrices[input.BoneIndices.x] * input.BoneWeights.x
                         + BoneMatrices[input.BoneIndices.y] * input.BoneWeights.y
                         + BoneMatrices[input.BoneIndices.z] * input.BoneWeights.z
                         + BoneMatrices[input.BoneIndices.w] * input.BoneWeights.w;
        pos = mul(skinMatrix, pos);
        norm = mul((float3x3)skinMatrix, norm);
        output.Normal = norm;
    }
    
    float4 worldPos = mul(World, pos);
    output.FragPos = worldPos.xyz;
    output.Position = mul(Projection, mul(View, worldPos));
    output.Color0 = input.Color0;
    output.Color1 = input.Color1;
    output.UV0 = input.TexCoord0;
    output.UV1 = input.TexCoord1;
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color0 = DiffuseMap0.Sample(LinearSampler, input.UV0);
    float4 color1 = DiffuseMap1.Sample(LinearSampler, input.UV0);
    float4 color2 = DiffuseMap2.Sample(LinearSampler, input.UV0);
    float4 color3 = DiffuseMap3.Sample(LinearSampler, input.UV0);
    
    float4 normal0 = NormalMap0.Sample(LinearSampler, input.UV0);
    float4 normal1 = NormalMap1.Sample(LinearSampler, input.UV0);
    float4 normal2 = NormalMap2.Sample(LinearSampler, input.UV0);
    float4 normal3 = NormalMap3.Sample(LinearSampler, input.UV0);
    
    float4 colorData;
    float4 normalData;
    
    if (UseLayerBlending == 1) {
        colorData = (color0 * input.Color1.b) + 
                    (color1 * input.Color1.g) + 
                    (color2 * input.Color1.r) + 
                    (color3 * input.Color1.a);
        normalData = (normal0 * input.Color1.b) + 
                     (normal1 * input.Color1.g) + 
                     (normal2 * input.Color1.r) + 
                     (normal3 * input.Color1.a);
    } else {
        colorData = color0;
        normalData = normal0;
    }
    
    float3 texNormal = UnpackNormalMap(normalData);
    
    if (colorData.a < 0.5) discard;
    
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(input.Normal), lightDir), 0.3);
    
    float3 baseColor = colorData.rgb * diff;
    float3 finalColor = lerp(baseColor, HighlightColor.rgb, HighlightColor.a);
    
    return float4(finalColor, 1.0);
}
)";
