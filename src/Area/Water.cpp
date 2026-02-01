#include "Water.h"
#include <d3dcompiler.h>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#pragma comment(lib, "d3dcompiler.lib")

bool gShowWater = true;
bool gWaterWireframe = false;

static bool sDebugMode = false;

static const char* sWaterShaderSource = R"(
cbuffer WaterCB : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 debugColor;
    float time;
    float3 padding;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float unk0 : TEXCOORD1;
    int unk1 : BLENDINDICES0;
    float4 blendMask : COLOR1;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 bitangent : TEXCOORD3;
    float2 uv : TEXCOORD4;
    float4 color : COLOR0;
    float unk0 : TEXCOORD5;
    nointerpolation int unk1 : BLENDINDICES0;
    float4 blendMask : COLOR1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(float4(input.position, 1.0), model);
    float4 viewPos = mul(worldPos, view);
    output.position = mul(viewPos, projection);

    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(float4(input.normal, 0.0), model).xyz);
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;
    output.uv = input.uv;
    output.color = input.color;
    output.unk0 = input.unk0;
    output.unk1 = input.unk1;
    output.blendMask = input.blendMask;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    if (debugColor.w > 0.5)
    {
        return float4(debugColor.rgb, 0.7);
    }

    float3 waterColor = input.color.rgb;

    float brightness = dot(waterColor, float3(0.299, 0.587, 0.114));
    if (brightness < 0.1)
    {
        waterColor = float3(0.2, 0.4, 0.8);
    }

    float fresnel = saturate(1.0 - abs(input.normal.y));
    waterColor = lerp(waterColor, float3(0.8, 0.9, 1.0), fresnel * 0.3);

    return float4(waterColor, 0.6);
}
)";

struct WaterCB
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 debugColor;
    float time;
    float padding[3];
};

namespace
{
    ComPtr<ID3D11VertexShader> sVertexShader;
    ComPtr<ID3D11PixelShader> sPixelShader;
    ComPtr<ID3D11InputLayout> sInputLayout;
    ComPtr<ID3D11Buffer> sConstantBuffer;
    ComPtr<ID3D11BlendState> sBlendState;
    ComPtr<ID3D11RasterizerState> sRasterizerState;
    ComPtr<ID3D11RasterizerState> sRasterizerStateWireframe;
    ComPtr<ID3D11DepthStencilState> sDepthState;
    ComPtr<ID3D11SamplerState> sSamplerState;
    bool sInitialized = false;
}

void WaterMesh::computeBounds()
{
    boundsMin = glm::vec3(FLT_MAX);
    boundsMax = glm::vec3(-FLT_MAX);

    for (const auto& v : vertices)
    {
        boundsMin.x = std::min(boundsMin.x, v.posX);
        boundsMin.y = std::min(boundsMin.y, v.posY);
        boundsMin.z = std::min(boundsMin.z, v.posZ);
        boundsMax.x = std::max(boundsMax.x, v.posX);
        boundsMax.y = std::max(boundsMax.y, v.posY);
        boundsMax.z = std::max(boundsMax.z, v.posZ);
    }
}

void WaterMesh::debugPrint() const
{
    std::cout << "[Water] WorldWaterTypeID: " << worldWaterTypeID << "\n";
    std::cout << "[Water] WaterLayerIDs: " << waterLayerIDs[0] << ", "
              << waterLayerIDs[1] << ", " << waterLayerIDs[2] << ", "
              << waterLayerIDs[3] << "\n";
    std::cout << "[Water] ShoreLineDistance: " << shoreLineDistance << "\n";
    std::cout << "[Water] ShoreLineWaterLayerID: " << shoreLineWaterLayerID << "\n";
    std::cout << "[Water] Indices: " << indexCount << ", Vertices: " << vertexCount << "\n";
    std::cout << "[Water] Bounds: (" << boundsMin.x << ", " << boundsMin.y << ", " << boundsMin.z
              << ") to (" << boundsMax.x << ", " << boundsMax.y << ", " << boundsMax.z << ")\n";

    if (!vertices.empty())
    {
        const auto& v0 = vertices[0];
        std::cout << "[Water] First vertex: pos=(" << v0.posX << ", " << v0.posY << ", " << v0.posZ << ")";
        std::cout << " color=(" << (int)v0.colorR << ", " << (int)v0.colorG << ", "
                  << (int)v0.colorB << ", " << (int)v0.colorA << ")\n";
    }

    std::cout << "[Water] GPU Ready: " << (gpuReady ? "yes" : "no") << "\n";
}

bool WaterMesh::loadFromRaw(const uint8_t* data, size_t size)
{
    if (!data || size < 80)
        return false;

    const uint8_t* ptr = data;

    memcpy(&worldWaterTypeID, ptr, 4); ptr += 4;
    for (int i = 0; i < 4; i++) { memcpy(&waterLayerIDs[i], ptr, 4); ptr += 4; }
    memcpy(&unk0, ptr, 4); ptr += 4;
    memcpy(&unk1, ptr, 4); ptr += 4;
    memcpy(&unk2, ptr, 4); ptr += 4;
    memcpy(&unk3, ptr, 4); ptr += 4;
    memcpy(&unk4, ptr, 4); ptr += 4;
    memcpy(&unk5, ptr, 4); ptr += 4;
    memcpy(&unk6, ptr, 4); ptr += 4;
    memcpy(&shoreLineDistance, ptr, 4); ptr += 4;
    memcpy(&unk7, ptr, 4); ptr += 4;
    memcpy(&shoreLineWaterLayerID, ptr, 4); ptr += 4;
    memcpy(&unk8, ptr, 4); ptr += 4;
    memcpy(&indexCount, ptr, 4); ptr += 4;
    memcpy(&vertexCount, ptr, 4); ptr += 4;
    memcpy(&unk9, ptr, 4); ptr += 4;
    memcpy(&unk10, ptr, 4); ptr += 4;

    if (indexCount > 1000000 || vertexCount > 500000)
        return false;

    if (indexCount == 0 || vertexCount == 0)
        return false;

    size_t headerSize = 80;
    size_t indicesSize = indexCount * sizeof(uint32_t);
    size_t verticesSize = vertexCount * sizeof(WaterVertex);
    size_t totalNeeded = headerSize + indicesSize + verticesSize;

    if (size < totalNeeded)
        return false;

    indices.resize(indexCount);
    memcpy(indices.data(), ptr, indicesSize);
    ptr += indicesSize;

    for (uint32_t i = 0; i < indexCount; i++)
    {
        if (indices[i] >= vertexCount)
            return false;
    }

    vertices.resize(vertexCount);
    memcpy(vertices.data(), ptr, verticesSize);

    computeBounds();

    return true;
}

bool WaterMesh::createGPUBuffers(ID3D11Device* device)
{
    if (vertices.empty() || indices.empty())
        return false;

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(WaterVertex));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    if (FAILED(device->CreateBuffer(&vbDesc, &vbData, vertexBuffer.GetAddressOf())))
        return false;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    if (FAILED(device->CreateBuffer(&ibDesc, &ibData, indexBuffer.GetAddressOf())))
        return false;

    gpuReady = true;
    return true;
}

namespace WaterRenderer
{

bool Initialize(ID3D11Device* device)
{
    if (sInitialized)
        return true;

    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    HRESULT hr = D3DCompile(sWaterShaderSource, strlen(sWaterShaderSource), "WaterShader",
        nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        vsBlob.GetAddressOf(), errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return false;
    }

    hr = D3DCompile(sWaterShaderSource, strlen(sWaterShaderSource), "WaterShader",
        nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
        psBlob.GetAddressOf(), errorBlob.GetAddressOf());

    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return false;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        nullptr, sVertexShader.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
        nullptr, sPixelShader.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     1, DXGI_FORMAT_R32_FLOAT,       0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32_SINT,        0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        1, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 68, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(layout, _countof(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), sInputLayout.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(WaterCB);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = device->CreateBuffer(&cbDesc, nullptr, sConstantBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&blendDesc, sBlendState.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;

    hr = device->CreateRasterizerState(&rsDesc, sRasterizerState.GetAddressOf());
    if (FAILED(hr)) return false;

    rsDesc.FillMode = D3D11_FILL_WIREFRAME;
    hr = device->CreateRasterizerState(&rsDesc, sRasterizerStateWireframe.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    hr = device->CreateDepthStencilState(&dsDesc, sDepthState.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.MaxAnisotropy = 1;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&sampDesc, sSamplerState.GetAddressOf());
    if (FAILED(hr)) return false;

    sInitialized = true;
    return true;
}

void Shutdown()
{
    sVertexShader.Reset();
    sPixelShader.Reset();
    sInputLayout.Reset();
    sConstantBuffer.Reset();
    sBlendState.Reset();
    sRasterizerState.Reset();
    sRasterizerStateWireframe.Reset();
    sDepthState.Reset();
    sSamplerState.Reset();
    sInitialized = false;
}

void SetDebugMode(bool enabled)
{
    sDebugMode = enabled;
}

void Render(ID3D11DeviceContext* context, WaterMesh* water,
            const glm::mat4& view, const glm::mat4& proj,
            const glm::vec3& areaOffset, float time)
{
    if (!gShowWater)
        return;

    if (!sInitialized || !water || !water->gpuReady)
        return;

    WaterCB cb;
    cb.model = glm::translate(glm::mat4(1.0f), areaOffset);
    cb.view = view;
    cb.projection = proj;
    cb.debugColor = sDebugMode ? glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.0f);
    cb.time = time;

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(sConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &cb, sizeof(cb));
        context->Unmap(sConstantBuffer.Get(), 0);
    }

    context->VSSetShader(sVertexShader.Get(), nullptr, 0);
    context->PSSetShader(sPixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, sConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, sConstantBuffer.GetAddressOf());
    context->IASetInputLayout(sInputLayout.Get());
    context->PSSetSamplers(0, 1, sSamplerState.GetAddressOf());

    context->OMSetBlendState(sBlendState.Get(), nullptr, 0xFFFFFFFF);
    context->RSSetState(gWaterWireframe ? sRasterizerStateWireframe.Get() : sRasterizerState.Get());
    context->OMSetDepthStencilState(sDepthState.Get(), 0);

    UINT stride = sizeof(WaterVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, water->vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(water->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(water->indexCount, 0, 0);

    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->RSSetState(nullptr);
    context->OMSetDepthStencilState(nullptr, 0);
}

}