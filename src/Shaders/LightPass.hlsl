struct FogParameters
{
    float3 color;
    float linearStart;
    float linearEnd;
    float density;
    int equation;
    int isEnabled;
};

struct SunParameters
{
    float3 color;
    float pad0;
    float3 direction;
    float intensity;
    int isEnabled;
    float3 pad1;
};

struct EnvironmentParameters
{
    float3 ambientColor;
    float pad0;
    FogParameters fogParams;
    SunParameters sunParams;
    int isEnabled;
    float3 pad1;
};

cbuffer LightPassCB : register(b0)
{
    EnvironmentParameters envParams;
    float3 viewPos;
    float pad0;
};

Texture2D gDiffuse : register(t0);
Texture2D gSpecular : register(t1);
Texture2D gNormal : register(t2);
Texture2D gMisc : register(t3);

SamplerState samplerClamp : register(s0);

struct VSInput
{
    float3 position : POSITION;
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

    output.position = float4(input.position, 1.0);
    output.uv = input.uv;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 Diffuse = gDiffuse.Sample(samplerClamp, input.uv);
    float4 Specular = gSpecular.Sample(samplerClamp, input.uv);
    float4 Normal = gNormal.Sample(samplerClamp, input.uv);
    return float4(Diffuse.xyz, 1.0);
}
