#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

enum class ShaderType
{
    Terrain,
    Water,
    Model,
    Wireframe,
    Skybox,
    Line,
    MapTile,
    Count
};

struct ShaderProgram
{
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> constantBuffer;
    std::string name;
    bool isValid = false;
};

struct InputLayoutDesc
{
    const D3D11_INPUT_ELEMENT_DESC* elements;
    UINT numElements;
};

class ShaderManager
{
public:
    static ShaderManager& Instance();
    
    bool Initialize(ID3D11Device* device);
    void Shutdown();
    
    ShaderProgram* GetShader(ShaderType type);
    
    bool ReloadShader(ShaderType type);
    void ReloadAllShaders();
    
    bool CompileShader(const std::string& source, const std::string& name,
                       const std::string& vsEntry, const std::string& psEntry,
                       const InputLayoutDesc& layoutDesc, ShaderProgram& outProgram);
    
    bool CompileShaderFromFile(const std::wstring& path, const std::string& name,
                               const std::string& vsEntry, const std::string& psEntry,
                               const InputLayoutDesc& layoutDesc, ShaderProgram& outProgram);
    
    bool CreateConstantBuffer(size_t size, ID3D11Buffer** outBuffer);
    
    ID3D11Device* GetDevice() const { return m_device; }
    
private:
    ShaderManager() = default;
    ~ShaderManager() = default;
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    
    bool LoadAllShaders();
    bool LoadShader(ShaderType type);
    std::string GetShaderSource(ShaderType type);
    InputLayoutDesc GetInputLayout(ShaderType type);
    
    ID3D11Device* m_device = nullptr;
    std::unordered_map<ShaderType, ShaderProgram> m_shaders;
    std::wstring m_shaderPath = L"shaders/";
    bool m_initialized = false;
};

namespace ShaderCB
{
    struct TerrainCB
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
        XMFLOAT4X4 model;
        XMFLOAT4 texScale;
        XMFLOAT4 highlightColor;
        XMFLOAT4 baseColor;
        XMFLOAT3 camPosition;
        int hasColorMap;
        
        XMFLOAT4 heightScale;
        XMFLOAT4 heightOffset;
        XMFLOAT4 parallaxScale;
        XMFLOAT4 parallaxOffset;
        
        XMFLOAT3 ambientColor;
        float pad0;
        XMFLOAT3 sunColor;
        float sunIntensity;
        XMFLOAT3 sunDirection;
        int sunEnabled;
        
        XMFLOAT3 fogColor;
        float fogStart;
        float fogEnd;
        float fogDensity;
        int fogEquation;
        int fogEnabled;
        
        int enableAreaGrid;
        int enableChunkGrid;
        XMFLOAT2 padEditor;
    };
    
    struct WaterCB
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
        XMFLOAT4X4 model;
        XMFLOAT3 areaOffset;
        float time;
        XMFLOAT3 camPosition;
        float pad0;
        
        XMFLOAT4 waterColor;
        XMFLOAT4 deepColor;
        XMFLOAT4 shallowColor;
        XMFLOAT4 waveParams;
        
        XMFLOAT3 sunDirection;
        float sunIntensity;
        XMFLOAT3 sunColor;
        float specularPower;
        
        XMFLOAT3 fogColor;
        float fogStart;
        float fogEnd;
        float fogDensity;
        int fogEquation;
        int fogEnabled;
    };
    
    struct ModelCB
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
        XMFLOAT4X4 model;
        XMFLOAT3 camPosition;
        float pad0;
        
        XMFLOAT3 lightPos;
        float pad1;
        XMFLOAT4 lightColor;
        XMFLOAT4 ambientColor;
        XMFLOAT4 objectColor;
        
        int hasAlphaTest;
        float alphaThreshold;
        int hasTwoSided;
        int pad2;
    };
    
    struct WireframeCB
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
        XMFLOAT4X4 model;
        XMFLOAT4 wireColor;
        XMFLOAT4 fillColor;
        float lineWidth;
        XMFLOAT3 pad;
    };
    
    struct SkyboxCB
    {
        XMFLOAT4X4 view;
        XMFLOAT4X4 projection;
        XMFLOAT4 sunDirection;
        XMFLOAT4 sunColor;
        XMFLOAT4 skyColorTop;
        XMFLOAT4 skyColorHorizon;
        XMFLOAT4 skyColorBottom;
        XMFLOAT4 fogColor;
        float time;
        float cloudDensity;
        float cloudSpeed;
        float pad;
    };
    
    struct LineCB
    {
        XMFLOAT4X4 viewProjection;
        XMFLOAT4 lineColor;
        float lineWidth;
        XMFLOAT3 pad;
    };
    
    struct MapTileCB
    {
        XMFLOAT4X4 viewProjection;
        XMFLOAT4X4 model;
        XMFLOAT4 tintColor;
        XMFLOAT2 uvOffset;
        XMFLOAT2 uvScale;
        float opacity;
        XMFLOAT3 pad;
    };
}

#define gShaderManager ShaderManager::Instance()