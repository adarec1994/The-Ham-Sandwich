#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "TerrainShader.h"

using Microsoft::WRL::ComPtr;

class AreaRender
{
	TerrainShader::ShaderResources mResources;
	bool mInitialized = false;

public:
	AreaRender();
	~AreaRender();

	void init(ID3D11Device* device);
	void bind(ID3D11DeviceContext* context);
	void updateConstants(ID3D11DeviceContext* context, const TerrainShader::TerrainCB& cb);

	[[nodiscard]] bool isInitialized() const { return mInitialized; }
	[[nodiscard]] const TerrainShader::ShaderResources& getResources() const { return mResources; }
	[[nodiscard]] ID3D11Buffer* getConstantBuffer() const { return mResources.constantBuffer.Get(); }
};