#pragma once

#include <cstdint>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace TerrainShader
{
    struct alignas(16) TerrainCB
    {
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
        DirectX::XMMATRIX model;
        DirectX::XMFLOAT4 texScale;
        DirectX::XMFLOAT4 highlightColor;
        DirectX::XMFLOAT4 baseColor;
        DirectX::XMFLOAT3 camPosition;
        int hasColorMap;
    };

    struct ShaderResources
    {
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3D11Buffer> constantBuffer;
        ComPtr<ID3D11SamplerState> samplerWrap;
        ComPtr<ID3D11SamplerState> samplerClamp;
    };

    bool CreateShaders(ID3D11Device* device, ShaderResources& out);
    void UpdateConstants(ID3D11DeviceContext* context, ID3D11Buffer* cb, const TerrainCB& data);

    extern const char* VertexSource;
    extern const char* FragmentSource;
}