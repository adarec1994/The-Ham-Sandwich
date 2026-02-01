#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

#pragma comment(lib, "d3dcompiler.lib")

namespace EmbeddedShaders
{
    #include "Terrain_embedded.inl"
    #include "Water_embedded.inl"
    #include "Model_embedded.inl"
    #include "Wireframe_embedded.inl"
    #include "Skybox_embedded.inl"
    #include "Line_embedded.inl"
    #include "MapTile_embedded.inl"
}

ShaderManager& ShaderManager::Instance()
{
    static ShaderManager instance;
    return instance;
}

bool ShaderManager::Initialize(ID3D11Device* device)
{
    if (!device)
        return false;
    
    m_device = device;
    
    if (!LoadAllShaders())
    {
        std::cerr << "ShaderManager: Failed to load shaders\n";
        return false;
    }
    
    m_initialized = true;
    return true;
}

void ShaderManager::Shutdown()
{
    m_shaders.clear();
    m_device = nullptr;
    m_initialized = false;
}

ShaderProgram* ShaderManager::GetShader(ShaderType type)
{
    auto it = m_shaders.find(type);
    if (it != m_shaders.end() && it->second.isValid)
        return &it->second;
    return nullptr;
}

bool ShaderManager::ReloadShader(ShaderType type)
{
    return LoadShader(type);
}

void ShaderManager::ReloadAllShaders()
{
    LoadAllShaders();
}

bool ShaderManager::LoadAllShaders()
{
    bool success = true;
    
    for (int i = 0; i < static_cast<int>(ShaderType::Count); ++i)
    {
        if (!LoadShader(static_cast<ShaderType>(i)))
        {
            success = false;
        }
    }
    
    return success;
}

bool ShaderManager::LoadShader(ShaderType type)
{
    std::string source = GetShaderSource(type);
    if (source.empty())
        return false;
    
    InputLayoutDesc layout = GetInputLayout(type);
    
    const char* name = nullptr;
    const char* vsEntry = "VSMain";
    const char* psEntry = "PSMain";
    
    switch (type)
    {
        case ShaderType::Terrain:   name = "Terrain"; break;
        case ShaderType::Water:     name = "Water"; break;
        case ShaderType::Model:     name = "Model"; break;
        case ShaderType::Wireframe: name = "Wireframe"; break;
        case ShaderType::Skybox:    name = "Skybox"; break;
        case ShaderType::Line:      name = "Line"; break;
        case ShaderType::MapTile:   name = "MapTile"; break;
        default: return false;
    }
    
    ShaderProgram program;
    if (!CompileShader(source, name, vsEntry, psEntry, layout, program))
    {
        std::cerr << "ShaderManager: Failed to compile " << name << " shader\n";
        return false;
    }
    
    m_shaders[type] = std::move(program);
    return true;
}

std::string ShaderManager::GetShaderSource(ShaderType type)
{
    switch (type)
    {
        case ShaderType::Terrain:   return EmbeddedShaders::TerrainShaderSource;
        case ShaderType::Water:     return EmbeddedShaders::WaterShaderSource;
        case ShaderType::Model:     return EmbeddedShaders::ModelShaderSource;
        case ShaderType::Wireframe: return EmbeddedShaders::WireframeShaderSource;
        case ShaderType::Skybox:    return EmbeddedShaders::SkyboxShaderSource;
        case ShaderType::Line:      return EmbeddedShaders::LineShaderSource;
        case ShaderType::MapTile:   return EmbeddedShaders::MapTileShaderSource;
        default: return "";
    }
}

InputLayoutDesc ShaderManager::GetInputLayout(ShaderType type)
{
    static D3D11_INPUT_ELEMENT_DESC terrainLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    static D3D11_INPUT_ELEMENT_DESC waterLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 2, DXGI_FORMAT_R32_SINT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 68, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    static D3D11_INPUT_ELEMENT_DESC modelLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R8G8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R8G8_UNORM, 0, 14, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BINORMAL", 0, DXGI_FORMAT_R8G8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 18, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 22, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 26, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 30, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R16G16_SNORM, 0, 34, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R16G16_SNORM, 0, 38, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    static D3D11_INPUT_ELEMENT_DESC wireframeLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    static D3D11_INPUT_ELEMENT_DESC skyboxLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    static D3D11_INPUT_ELEMENT_DESC lineLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    static D3D11_INPUT_ELEMENT_DESC mapTileLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    switch (type)
    {
        case ShaderType::Terrain:   return {terrainLayout, 4};
        case ShaderType::Water:     return {waterLayout, 9};
        case ShaderType::Model:     return {modelLayout, 10};
        case ShaderType::Wireframe: return {wireframeLayout, 2};
        case ShaderType::Skybox:    return {skyboxLayout, 1};
        case ShaderType::Line:      return {lineLayout, 2};
        case ShaderType::MapTile:   return {mapTileLayout, 2};
        default: return {nullptr, 0};
    }
}

bool ShaderManager::CompileShader(const std::string& source, const std::string& name,
                                   const std::string& vsEntry, const std::string& psEntry,
                                   const InputLayoutDesc& layoutDesc, ShaderProgram& outProgram)
{
    if (!m_device || source.empty())
        return false;
    
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> vsError;
    HRESULT hr = D3DCompile(source.c_str(), source.length(), name.c_str(), nullptr, nullptr,
                            vsEntry.c_str(), "vs_5_0", flags, 0, &vsBlob, &vsError);
    if (FAILED(hr))
    {
        if (vsError)
            std::cerr << "VS Error (" << name << "): " << (char*)vsError->GetBufferPointer() << "\n";
        return false;
    }
    
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> psError;
    hr = D3DCompile(source.c_str(), source.length(), name.c_str(), nullptr, nullptr,
                    psEntry.c_str(), "ps_5_0", flags, 0, &psBlob, &psError);
    if (FAILED(hr))
    {
        if (psError)
            std::cerr << "PS Error (" << name << "): " << (char*)psError->GetBufferPointer() << "\n";
        return false;
    }
    
    hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                      nullptr, &outProgram.vertexShader);
    if (FAILED(hr))
        return false;
    
    hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                     nullptr, &outProgram.pixelShader);
    if (FAILED(hr))
        return false;
    
    if (layoutDesc.elements && layoutDesc.numElements > 0)
    {
        hr = m_device->CreateInputLayout(layoutDesc.elements, layoutDesc.numElements,
                                         vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                         &outProgram.inputLayout);
        if (FAILED(hr))
            return false;
    }
    
    outProgram.name = name;
    outProgram.isValid = true;
    
    return true;
}

bool ShaderManager::CompileShaderFromFile(const std::wstring& path, const std::string& name,
                                           const std::string& vsEntry, const std::string& psEntry,
                                           const InputLayoutDesc& layoutDesc, ShaderProgram& outProgram)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "ShaderManager: Could not open file: " << name << "\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    return CompileShader(source, name, vsEntry, psEntry, layoutDesc, outProgram);
}

bool ShaderManager::CreateConstantBuffer(size_t size, ID3D11Buffer** outBuffer)
{
    if (!m_device || !outBuffer)
        return false;
    
    size = (size + 15) & ~15;
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(size);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    return SUCCEEDED(m_device->CreateBuffer(&desc, nullptr, outBuffer));
}