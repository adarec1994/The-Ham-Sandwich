#include "TerrainShader.h"
#include "Settings.h"

namespace TerrainShader
{
    const char* VertexSource = R"(
cbuffer TerrainCB : register(b0)
{
    row_major matrix view;
    row_major matrix projection;
    row_major matrix model;
    float4 texScale;
    float4 highlightColor;
    float4 baseColor;
    float3 camPosition;
    int hasColorMap;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float2 blendCoord : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 bitangent : TEXCOORD5;
};

PSInput main(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(float4(input.position, 1.0), model);
    output.fragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)model;
    output.normal = normalize(mul(normalMatrix, input.normal));

    float3 T = normalize(mul(normalMatrix, input.tangent.xyz));
    float3 N = output.normal;
    float3 B = cross(N, T) * input.tangent.w;

    output.tangent = T;
    output.bitangent = B;

    output.texCoord = input.texCoord;
    output.blendCoord = input.texCoord;

    float4 viewPos = mul(view, worldPos);
    output.position = mul(projection, viewPos);

    return output;
}
)";

    const char* FragmentSource = R"(
cbuffer TerrainCB : register(b0)
{
    row_major matrix view;
    row_major matrix projection;
    row_major matrix model;
    float4 texScale;
    float4 highlightColor;
    float4 baseColor;
    float3 camPosition;
    int hasColorMap;
};

Texture2D blendMap : register(t0);
Texture2D colorMap : register(t1);
Texture2D layer0 : register(t2);
Texture2D layer1 : register(t3);
Texture2D layer2 : register(t4);
Texture2D layer3 : register(t5);
Texture2D layer0Normal : register(t6);
Texture2D layer1Normal : register(t7);
Texture2D layer2Normal : register(t8);
Texture2D layer3Normal : register(t9);

SamplerState samplerWrap : register(s0);
SamplerState samplerClamp : register(s1);
SamplerState samplerNormal : register(s2);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float2 blendCoord : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 bitangent : TEXCOORD5;
};

float3 sampleNormal(Texture2D normalTex, float2 uv)
{
    float3 n = normalTex.Sample(samplerWrap, uv).rgb;
    return normalize(n * 2.0 - 1.0);
}

float4 main(PSInput input) : SV_TARGET
{
    float4 blend = blendMap.Sample(samplerClamp, input.blendCoord);

    float blendSum = blend.r + blend.g + blend.b + blend.a;
    if (blendSum > 0.001)
        blend /= blendSum;
    else
        blend = float4(1.0, 0.0, 0.0, 0.0);

    float2 uv0 = input.texCoord * texScale.x;
    float2 uv1 = input.texCoord * texScale.y;
    float2 uv2 = input.texCoord * texScale.z;
    float2 uv3 = input.texCoord * texScale.w;

    float4 col0 = layer0.Sample(samplerWrap, uv0);
    float4 col1 = layer1.Sample(samplerWrap, uv1);
    float4 col2 = layer2.Sample(samplerWrap, uv2);
    float4 col3 = layer3.Sample(samplerWrap, uv3);

    float4 diffuse = col0 * blend.r + col1 * blend.g + col2 * blend.b + col3 * blend.a;

    float3 n0 = sampleNormal(layer0Normal, uv0);
    float3 n1 = sampleNormal(layer1Normal, uv1);
    float3 n2 = sampleNormal(layer2Normal, uv2);
    float3 n3 = sampleNormal(layer3Normal, uv3);

    float3 blendedNormal = normalize(n0 * blend.r + n1 * blend.g + n2 * blend.b + n3 * blend.a);

    float3 N = normalize(input.normal);

    float3 worldNormal = normalize(float3(
        N.x + blendedNormal.x * 0.15,
        N.y,
        N.z + blendedNormal.y * 0.15
    ));

    if (hasColorMap > 0)
    {
        float4 tint = colorMap.Sample(samplerClamp, input.blendCoord);
        diffuse.rgb *= tint.rgb * 2.0;
    }

    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float NdotL = max(dot(worldNormal, lightDir), 0.0);

    float3 ambient = float3(0.3, 0.3, 0.3);
    float3 lighting = ambient + float3(0.7, 0.7, 0.7) * NdotL;

    float3 finalColor = diffuse.rgb * lighting * baseColor.rgb;

    if (highlightColor.a > 0.0)
    {
        finalColor = lerp(finalColor, highlightColor.rgb, highlightColor.a * 0.3);
    }

    return float4(finalColor, 1.0);
}
)";

    bool CreateShaders(ID3D11Device* device, ShaderResources& out)
    {
        if (!device) return false;

        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG;
#endif

        ComPtr<ID3DBlob> vsBlob;
        ComPtr<ID3DBlob> vsError;
        HRESULT hr = D3DCompile(VertexSource, strlen(VertexSource), "TerrainVS", nullptr, nullptr,
                                "main", "vs_5_0", flags, 0, &vsBlob, &vsError);
        if (FAILED(hr)) return false;

        ComPtr<ID3DBlob> psBlob;
        ComPtr<ID3DBlob> psError;
        hr = D3DCompile(FragmentSource, strlen(FragmentSource), "TerrainPS", nullptr, nullptr,
                        "main", "ps_5_0", flags, 0, &psBlob, &psError);
        if (FAILED(hr)) return false;

        hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                        nullptr, &out.vertexShader);
        if (FAILED(hr)) return false;

        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                       nullptr, &out.pixelShader);
        if (FAILED(hr)) return false;

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        hr = device->CreateInputLayout(layout, 4, vsBlob->GetBufferPointer(),
                                       vsBlob->GetBufferSize(), &out.inputLayout);
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(TerrainCB);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&cbDesc, nullptr, &out.constantBuffer);
        if (FAILED(hr)) return false;

        const GraphicsSettings& gfxSettings = GetGraphicsSettings();

        D3D11_SAMPLER_DESC sampDesc = {};

        if (gfxSettings.anisotropicFiltering)
        {
            sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            sampDesc.MaxAnisotropy = gfxSettings.anisotropicLevel;
        }
        else
        {
            sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sampDesc.MaxAnisotropy = 1;
        }
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        sampDesc.MipLODBias = 0.0f;

        hr = device->CreateSamplerState(&sampDesc, &out.samplerWrap);
        if (FAILED(hr)) return false;

        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.MaxAnisotropy = 1;
        sampDesc.MipLODBias = 0.0f;

        hr = device->CreateSamplerState(&sampDesc, &out.samplerClamp);
        if (FAILED(hr)) return false;

        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.MaxAnisotropy = 1;
        sampDesc.MipLODBias = 0.0f;

        hr = device->CreateSamplerState(&sampDesc, &out.samplerNormal);
        if (FAILED(hr)) return false;

        return true;
    }

    void UpdateConstants(ID3D11DeviceContext* context, ID3D11Buffer* cb, const TerrainCB& data)
    {
        if (!context || !cb) return;

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = context->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, &data, sizeof(TerrainCB));
            context->Unmap(cb, 0);
        }
    }
}