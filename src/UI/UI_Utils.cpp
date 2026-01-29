#include "UI_Utils.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <codecvt>
#include <locale>

#include <d3d11.h>
#include <stb_image.h>

#ifdef _WIN32
#include <windows.h>
#endif

extern ID3D11Device* gDevice;
extern ID3D11DeviceContext* gContext;

std::string wstring_to_utf8(const std::wstring& str)
{
    if (str.empty()) return {};
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0, NULL, NULL);
    std::string out(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), out.data(), size_needed, NULL, NULL);
    return out;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(str);
#endif
}

std::string ToLowerCopy(std::string s)
{
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool EndsWithNoCase(const std::string& s, const std::string& suffix)
{
    if (s.size() < suffix.size()) return false;
    const size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        const auto a = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(s[off + i])));
        const auto b = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(suffix[i])));
        if (a != b) return false;
    }
    return true;
}

std::string GetExtLower(const std::string& name)
{
    std::filesystem::path p(name);
    std::string e = p.extension().string();
    if (e.empty()) return "";
    return ToLowerCopy(e);
}

bool ContainsLowerFast(const std::string& hayLower, const std::string& needleLower)
{
    if (needleLower.empty()) return true;
    return hayLower.find(needleLower) != std::string::npos;
}

namespace UI_Utils {

    ImTextureID LoadTexture(const char* filename)
    {
        if (!gDevice) return (ImTextureID)0;

        int width, height, nrChannels;
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 4);

        if (!data)
            return (ImTextureID)0;

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* texture = nullptr;
        HRESULT hr = gDevice->CreateTexture2D(&texDesc, &initData, &texture);
        stbi_image_free(data);

        if (FAILED(hr) || !texture)
            return (ImTextureID)0;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = gDevice->CreateShaderResourceView(texture, &srvDesc, &srv);
        texture->Release();

        if (FAILED(hr))
            return (ImTextureID)0;

        return reinterpret_cast<ImTextureID>(srv);
    }

    ImTextureID LoadTextureFromResource(int resourceId)
    {
#ifdef _WIN32
        if (!gDevice) return (ImTextureID)0;

        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hRes) return (ImTextureID)0;

        HGLOBAL hMem = LoadResource(nullptr, hRes);
        if (!hMem) return (ImTextureID)0;

        DWORD dataSize = SizeofResource(nullptr, hRes);
        const void* data = LockResource(hMem);
        if (!data) return (ImTextureID)0;

        int width, height;
        unsigned char* imageData = stbi_load_from_memory(
            static_cast<const unsigned char*>(data),
            static_cast<int>(dataSize),
            &width, &height, nullptr, 4);

        if (!imageData)
            return (ImTextureID)0;

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = imageData;
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* texture = nullptr;
        HRESULT hr = gDevice->CreateTexture2D(&texDesc, &initData, &texture);
        stbi_image_free(imageData);

        if (FAILED(hr) || !texture)
            return (ImTextureID)0;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = gDevice->CreateShaderResourceView(texture, &srvDesc, &srv);
        texture->Release();

        if (FAILED(hr))
            return (ImTextureID)0;

        return reinterpret_cast<ImTextureID>(srv);
#else
        (void)resourceId;
        return (ImTextureID)0;
#endif
    }
}