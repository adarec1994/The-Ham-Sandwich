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
}

void AreaRender::bind(ID3D11DeviceContext* context)
{
    if (!mInitialized || !context) return;

    context->IASetInputLayout(mResources.inputLayout.Get());
    context->VSSetShader(mResources.vertexShader.Get(), nullptr, 0);
    context->PSSetShader(mResources.pixelShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, mResources.constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, mResources.constantBuffer.GetAddressOf());

    ID3D11SamplerState* samplers[] = {mResources.samplerWrap.Get(), mResources.samplerClamp.Get()};
    context->PSSetSamplers(0, 2, samplers);
}

void AreaRender::updateConstants(ID3D11DeviceContext* context, const TerrainShader::TerrainCB& cb)
{
    if (!mInitialized || !context) return;
    TerrainShader::UpdateConstants(context, mResources.constantBuffer.Get(), cb);
}