#include "UI.h"
#include "../Area/AreaRender.h"
#include "../resource.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>
#include <iostream>
#include <fstream>

static ID3D11Device* sDevice = nullptr;
static ID3D11DeviceContext* sContext = nullptr;

void SetUIDevice(ID3D11Device* device, ID3D11DeviceContext* context)
{
    sDevice = device;
    sContext = context;
}

static bool GetResourceData(int resourceId, const void** outData, DWORD* outSize)
{
    HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(nullptr, hRes);
    if (!hMem) return false;
    *outSize = SizeofResource(nullptr, hRes);
    *outData = LockResource(hMem);
    return *outData != nullptr;
}

struct GridConstantBuffer
{
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

void InitGrid(AppState& state)
{
    if (!state.device) return;

    const char* vertexShaderSource = R"(
        cbuffer GridCB : register(b0)
        {
            matrix view;
            matrix projection;
        };

        struct VSInput
        {
            float3 position : POSITION;
        };

        struct PSInput
        {
            float4 position : SV_POSITION;
        };

        PSInput main(VSInput input)
        {
            PSInput output;
            float4 worldPos = float4(input.position, 1.0);
            output.position = mul(projection, mul(view, worldPos));
            return output;
        }
    )";

    const char* pixelShaderSource = R"(
        struct PSInput
        {
            float4 position : SV_POSITION;
        };

        float4 main(PSInput input) : SV_TARGET
        {
            return float4(0.4, 0.4, 0.4, 1.0);
        }
    )";

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> vsError;
    HRESULT hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), "GridVS", nullptr, nullptr,
                            "main", "vs_5_0", flags, 0, &vsBlob, &vsError);
    if (FAILED(hr))
    {
        if (vsError)
            std::cout << "ERROR::VERTEX_SHADER::COMPILATION_FAILED\n" << (char*)vsError->GetBufferPointer() << std::endl;
        return;
    }

    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> psError;
    hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), "GridPS", nullptr, nullptr,
                    "main", "ps_5_0", flags, 0, &psBlob, &psError);
    if (FAILED(hr))
    {
        if (psError)
            std::cout << "ERROR::PIXEL_SHADER::COMPILATION_FAILED\n" << (char*)psError->GetBufferPointer() << std::endl;
        return;
    }

    hr = state.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                          nullptr, &state.grid.VertexShader);
    if (FAILED(hr)) return;

    hr = state.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                         nullptr, &state.grid.PixelShader);
    if (FAILED(hr)) return;

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    hr = state.device->CreateInputLayout(layout, _countof(layout),
                                         vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                         &state.grid.InputLayout);
    if (FAILED(hr)) return;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(GridConstantBuffer);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = state.device->CreateBuffer(&cbDesc, nullptr, &state.grid.ConstantBuffer);
    if (FAILED(hr)) return;

    std::vector<float> vertices;

    constexpr int size = 20;
    for (int i = -size; i <= size; ++i)
    {
        constexpr float step = 1.0f;

        const float fi = static_cast<float>(i) * step;
        constexpr float fs = static_cast<float>(size) * step;

        vertices.push_back(fi);   vertices.push_back(0.0f); vertices.push_back(-fs);
        vertices.push_back(fi);   vertices.push_back(0.0f); vertices.push_back( fs);

        vertices.push_back(-fs);  vertices.push_back(0.0f); vertices.push_back(fi);
        vertices.push_back( fs);  vertices.push_back(0.0f); vertices.push_back(fi);
    }

    state.grid.VertexCount = static_cast<int>(vertices.size() / 3);

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(float));
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

    hr = state.device->CreateBuffer(&vbDesc, &initData, &state.grid.VertexBuffer);
    if (FAILED(hr)) return;
}

void RenderGrid(AppState& state, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection)
{
    if (!state.context || !state.grid.VertexBuffer || !state.grid.VertexShader) return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = state.context->Map(state.grid.ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        GridConstantBuffer* cb = static_cast<GridConstantBuffer*>(mapped.pData);
        cb->view = view;
        cb->projection = projection;
        state.context->Unmap(state.grid.ConstantBuffer.Get(), 0);
    }

    state.context->IASetInputLayout(state.grid.InputLayout.Get());
    state.context->VSSetShader(state.grid.VertexShader.Get(), nullptr, 0);
    state.context->PSSetShader(state.grid.PixelShader.Get(), nullptr, 0);
    state.context->VSSetConstantBuffers(0, 1, state.grid.ConstantBuffer.GetAddressOf());

    UINT stride = 3 * sizeof(float);
    UINT offset = 0;
    state.context->IASetVertexBuffers(0, 1, state.grid.VertexBuffer.GetAddressOf(), &stride, &offset);
    state.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    state.context->Draw(state.grid.VertexCount, 0);
}

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_texture, int* out_width, int* out_height)
{
    if (!sDevice) return false;

    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, nullptr, 4);
    if (image_data == nullptr)
        return false;

    for (int i = 0; i < image_width * image_height * 4; i += 4)
    {
        unsigned char alpha = image_data[i + 3];
        if (alpha > 0)
        {
            image_data[i]     = 255 - image_data[i];
            image_data[i + 1] = 255 - image_data[i + 1];
            image_data[i + 2] = 255 - image_data[i + 2];
        }
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = image_width;
    texDesc.Height = image_height;
    texDesc.MipLevels = 0;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = sDevice->CreateTexture2D(&texDesc, nullptr, &texture);
    if (FAILED(hr))
    {
        stbi_image_free(image_data);
        return false;
    }

    sContext->UpdateSubresource(texture.Get(), 0, nullptr, image_data, image_width * 4, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);

    hr = sDevice->CreateShaderResourceView(texture.Get(), &srvDesc, out_texture);
    if (FAILED(hr))
    {
        stbi_image_free(image_data);
        return false;
    }

    sContext->GenerateMips(*out_texture);

    stbi_image_free(image_data);

    *out_width = image_width;
    *out_height = image_height;

    return true;
}

static const char* GetFontPath(int resourceId)
{
    switch (resourceId)
    {
        case IDR_FONT_ROBOTO: return "assets/fonts/Roboto-Regular.ttf";
        default:              return nullptr;
    }
}

static bool LoadFontFromResourceCrossPlat(int resourceId, ImGuiIO& io, float fontSize)
{
    const void* fontData = nullptr;
    DWORD fontDataSize = 0;
    if (GetResourceData(resourceId, &fontData, &fontDataSize))
    {
        void* fontDataCopy = IM_ALLOC(fontDataSize);
        memcpy(fontDataCopy, fontData, fontDataSize);
        ImFont* font = io.Fonts->AddFontFromMemoryTTF(fontDataCopy, static_cast<int>(fontDataSize), fontSize);
        if (font != nullptr)
            return true;
    }
    const char* path = GetFontPath(resourceId);
    if (path)
    {
        ImFont* font = io.Fonts->AddFontFromFileTTF(path, fontSize);
        if (font != nullptr)
            return true;
    }
    return false;
}

void ApplyBrainwaveStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding     = ImVec2(12, 12);
    style.FramePadding      = ImVec2(8, 6);
    style.ItemSpacing       = ImVec2(8, 8);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.IndentSpacing     = 25.0f;
    style.ScrollbarSize     = 12.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize       = 5.0f;
    style.GrabRounding      = 3.0f;
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 4.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg]       = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]        = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Border]         = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.21f, 0.22f, 0.80f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]        = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]  = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
    colors[ImGuiCol_MenuBarBg]      = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.21f, 0.22f, 0.40f);
    colors[ImGuiCol_ButtonHovered]  = ImVec4(0.25f, 0.26f, 0.27f, 0.70f);
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.30f, 0.31f, 0.32f, 1.00f);
    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_Separator]      = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
}

void InitUI(AppState& state)
{
    ApplyBrainwaveStyle();

    ImGuiIO& io = ImGui::GetIO();

    if (!LoadFontFromResourceCrossPlat(IDR_FONT_ROBOTO, io, 18.0f))
    {
        io.Fonts->AddFontDefault();
    }

    sDevice = state.device;
    sContext = state.context;

    state.areaRender = std::make_shared<AreaRender>();
    state.areaRender->init(state.device);
}