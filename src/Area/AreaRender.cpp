#include "AreaRender.h"
#include "AreaFile.h"
#include "TerrainShader.h"
#include <vector>
#include <string>

AreaRender::AreaRender() = default;

AreaRender::~AreaRender() = default;

void AreaRender::init(ID3D11Device* device)
{
    if (!device) return;

    mResources = TerrainShader::ShaderResources();
    mInitialized = TerrainShader::CreateShaders(device, mResources);

    if (!mInitialized) return;

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable = FALSE;
    rasterDesc.MultisampleEnable = FALSE;
    rasterDesc.AntialiasedLineEnable = FALSE;

    HRESULT hr = device->CreateRasterizerState(&rasterDesc, &mRasterState);
    if (FAILED(hr))
    {
        mInitialized = false;
        return;
    }

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthDesc.StencilEnable = FALSE;

    hr = device->CreateDepthStencilState(&depthDesc, &mDepthState);
    if (FAILED(hr))
    {
        mInitialized = false;
        return;
    }

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&blendDesc, &mBlendState);
    if (FAILED(hr))
    {
        mInitialized = false;
        return;
    }
}

void AreaRender::bind(ID3D11DeviceContext* context)
{
    if (!mInitialized || !context) return;

    context->IASetInputLayout(mResources.inputLayout.Get());
    context->VSSetShader(mResources.vertexShader.Get(), nullptr, 0);
    context->PSSetShader(mResources.pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, mResources.constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, mResources.constantBuffer.GetAddressOf());

    ID3D11SamplerState* samplers[] = {mResources.samplerWrap.Get(), mResources.samplerClamp.Get(), mResources.samplerNormal.Get()};
    context->PSSetSamplers(0, 3, samplers);

    context->RSSetState(mRasterState.Get());
    context->OMSetDepthStencilState(mDepthState.Get(), 0);

    float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context->OMSetBlendState(mBlendState.Get(), blendFactor, 0xFFFFFFFF);
}

void AreaRender::updateConstants(ID3D11DeviceContext* context, const TerrainShader::TerrainCB& cb)
{
    if (!mInitialized || !context) return;
    TerrainShader::UpdateConstants(context, mResources.constantBuffer.Get(), cb);
}