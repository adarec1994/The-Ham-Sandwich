#define NOMINMAX
#include "TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include "../Database/Tbl.h"
#include "../Database/TblReader.h"
#include "../Database/Definitions/WorldLayer.h"
#include <cstring>
#include <functional>
#include <cwctype>
#include <algorithm>
#include <cctype>

namespace TerrainTexture
{
    Manager& Manager::Instance()
    {
        static Manager instance;
        return instance;
    }

    Manager::~Manager()
    {
        ClearCache();
    }

    void Manager::SetDevice(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        mDevice = device;
        mContext = context;
    }

    void Manager::ClearCache()
    {
        mPathTextureCache.clear();
        mTextureCache.clear();
        mFallbackWhite.Reset();
        mFallbackNormal.Reset();
    }

    static std::wstring ToLowerW(const std::wstring& s)
    {
        std::wstring result;
        result.reserve(s.size());
        for (wchar_t c : s)
            result += static_cast<wchar_t>(std::towlower(c));
        return result;
    }

    static FileEntryPtr FindFileRecursive(const IFileSystemEntryPtr& entry, const std::wstring& targetLower)
    {
        if (!entry) return nullptr;

        if (!entry->isDirectory())
        {
            std::wstring name = entry->getEntryName();
            std::wstring nameLower = ToLowerW(name);
            if (nameLower.find(targetLower) != std::wstring::npos)
            {
                return std::dynamic_pointer_cast<FileEntry>(entry);
            }
            return nullptr;
        }

        for (const auto& child : entry->getChildren())
        {
            auto result = FindFileRecursive(child, targetLower);
            if (result) return result;
        }
        return nullptr;
    }

    static std::string WideToNarrow(const std::wstring& wide)
    {
        std::string result;
        result.reserve(wide.size());
        for (wchar_t c : wide)
        {
            if (c < 128)
                result += static_cast<char>(c);
            else
                result += '?';
        }
        return result;
    }

    bool Manager::LoadWorldLayerTable(const ArchivePtr& archive)
    {
        if (mTableLoaded) return true;
        if (!archive) return false;

        auto root = archive->getRoot();
        if (!root) return false;

        auto fileEntry = FindFileRecursive(root, L"worldlayer.tbl");
        if (!fileEntry)
        {
            mTableLoaded = true;
            return false;
        }

        std::vector<uint8_t> data;
        if (!archive->getFileData(fileEntry, data))
        {
            mTableLoaded = true;
            return false;
        }

        Tbl::Table<Database::Definitions::WorldLayer> tbl;
        if (!tbl.load(data.data(), data.size()))
        {
            mTableLoaded = true;
            return false;
        }

        for (const auto& [id, tblEntry] : tbl.entries())
        {
            WorldLayerEntry entry;
            entry.id = tblEntry.ID;
            entry.diffusePath = WideToNarrow(tblEntry.ColorMapPath);
            entry.normalPath = WideToNarrow(tblEntry.NormalMapPath);
            entry.scaleU = tblEntry.MetersPerTextureTile;
            entry.scaleV = tblEntry.MetersPerTextureTile;
            mLayerTable[id] = entry;
        }

        mTableLoaded = true;
        return true;
    }

    const WorldLayerEntry* Manager::GetLayerEntry(uint32_t layerId) const
    {
        auto it = mLayerTable.find(layerId);
        if (it != mLayerTable.end())
            return &it->second;
        return nullptr;
    }

    static FileEntryPtr FindFileByPath(const IFileSystemEntryPtr& root, const std::wstring& wpath)
    {
        if (!root || wpath.empty()) return nullptr;

        std::wstring remaining = wpath;
        IFileSystemEntryPtr current = root;

        while (!remaining.empty() && current && current->isDirectory())
        {
            size_t sep = remaining.find_first_of(L"\\/");
            std::wstring component = (sep != std::wstring::npos) ? remaining.substr(0, sep) : remaining;
            remaining = (sep != std::wstring::npos) ? remaining.substr(sep + 1) : L"";

            std::wstring componentLower = ToLowerW(component);

            IFileSystemEntryPtr found = nullptr;
            for (const auto& child : current->getChildren())
            {
                if (!child) continue;
                std::wstring childLower = ToLowerW(child->getEntryName());
                if (childLower == componentLower)
                {
                    found = child;
                    break;
                }
            }

            if (!found) return nullptr;

            if (remaining.empty())
            {
                return std::dynamic_pointer_cast<FileEntry>(found);
            }

            current = found;
        }

        return nullptr;
    }

    bool Manager::LoadRawTextureFromPath(const ArchivePtr& archive, const std::string& path, RawTextureData& outData)
    {
        if (!archive || path.empty()) return false;

        FileEntryPtr fileEntry = archive->findFileCached(path);

        if (!fileEntry)
        {
            std::wstring wpath(path.begin(), path.end());
            auto root = archive->getRoot();
            if (root)
            {
                fileEntry = FindFileByPath(root, wpath);
            }
        }

        if (!fileEntry)
        {
            auto root = archive->getRoot();
            if (root)
            {
                size_t lastSlash = path.rfind('/');
                if (lastSlash == std::string::npos) lastSlash = path.rfind('\\');
                std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
                std::wstring wfn(filename.begin(), filename.end());
                std::wstring wfnLower = ToLowerW(wfn);
                fileEntry = FindFileRecursive(root, wfnLower);
            }
        }

        if (!fileEntry) return false;

        std::vector<uint8_t> bytes;
        if (!archive->getFileData(fileEntry, bytes)) return false;
        if (bytes.empty()) return false;

        Tex::File tf;
        if (!tf.readFromMemory(bytes.data(), bytes.size())) return false;

        Tex::ImageRGBA img;
        if (!tf.decodeLargestMipToRGBA(img)) return false;

        outData.rgba = std::move(img.rgba);
        outData.width = img.width;
        outData.height = img.height;
        outData.valid = true;

        return true;
    }

    bool Manager::GetLayerTextureData(const ArchivePtr& archive, uint32_t layerId, RawTextureData& outData)
    {
        if (!archive || layerId == 0) return false;

        if (!LoadWorldLayerTable(archive)) return false;

        const WorldLayerEntry* entry = GetLayerEntry(layerId);
        if (!entry || entry->diffusePath.empty()) return false;

        return LoadRawTextureFromPath(archive, entry->diffusePath, outData);
    }

    bool Manager::LoadTextureFromPath(const ArchivePtr& archive, const std::string& path,
                                      ComPtr<ID3D11ShaderResourceView>& outSRV, int& outW, int& outH)
    {
        if (!archive || path.empty() || !mDevice) return false;

        auto cacheIt = mPathTextureCache.find(path);
        if (cacheIt != mPathTextureCache.end())
        {
            outSRV = cacheIt->second.srv;
            outW = cacheIt->second.width;
            outH = cacheIt->second.height;
            return outSRV != nullptr;
        }

        RawTextureData rawData;
        if (!LoadRawTextureFromPath(archive, path, rawData))
        {
            mPathTextureCache[path] = {nullptr, 0, 0};
            return false;
        }

        outSRV = UploadRGBATexture(mDevice, mContext, rawData.rgba.data(), rawData.width, rawData.height, true);
        outW = rawData.width;
        outH = rawData.height;

        mPathTextureCache[path] = {outSRV, outW, outH};

        return outSRV != nullptr;
    }

    void Manager::EnsureFallbackTextures()
    {
        if (!mDevice) return;

        if (!mFallbackWhite)
        {
            uint8_t white[4] = {255, 255, 255, 255};
            mFallbackWhite = UploadRGBATexture(mDevice, mContext, white, 1, 1, false);
        }
        if (!mFallbackNormal)
        {
            uint8_t normal[4] = {128, 128, 255, 255};
            mFallbackNormal = UploadRGBATexture(mDevice, mContext, normal, 1, 1, false);
        }
    }

    ID3D11ShaderResourceView* Manager::GetFallbackWhite()
    {
        EnsureFallbackTextures();
        return mFallbackWhite.Get();
    }

    ID3D11ShaderResourceView* Manager::GetFallbackNormal()
    {
        EnsureFallbackTextures();
        return mFallbackNormal.Get();
    }

    const CachedTexture* Manager::GetLayerTexture(const ArchivePtr& archive, uint32_t layerId)
    {
        auto cacheIt = mTextureCache.find(layerId);
        if (cacheIt != mTextureCache.end() && cacheIt->second.loaded)
            return &cacheIt->second;

        EnsureFallbackTextures();

        const WorldLayerEntry* entry = GetLayerEntry(layerId);
        if (!entry)
        {
            CachedTexture tex;
            tex.diffuse = mFallbackWhite;
            tex.normal = mFallbackNormal;
            tex.width = 1;
            tex.height = 1;
            tex.loaded = true;
            mTextureCache[layerId] = tex;
            return &mTextureCache[layerId];
        }

        CachedTexture tex;

        int w = 0, h = 0;
        if (!entry->diffusePath.empty())
        {
            if (LoadTextureFromPath(archive, entry->diffusePath, tex.diffuse, w, h))
            {
                tex.width = w;
                tex.height = h;
            }
        }

        if (!entry->normalPath.empty())
        {
            int nw = 0, nh = 0;
            LoadTextureFromPath(archive, entry->normalPath, tex.normal, nw, nh);
        }

        if (!tex.diffuse)
        {
            tex.diffuse = mFallbackWhite;
            tex.width = 1;
            tex.height = 1;
        }

        if (!tex.normal)
        {
            tex.normal = mFallbackNormal;
        }

        tex.loaded = true;
        mTextureCache[layerId] = tex;
        return &mTextureCache[layerId];
    }

    ComPtr<ID3D11ShaderResourceView> Manager::CreateBlendMapTexture(const uint8_t* data, int width, int height)
    {
        if (!data || width <= 0 || height <= 0 || !mDevice) return nullptr;
        return UploadRGBATexture(mDevice, mContext, data, width, height, false);
    }

    ComPtr<ID3D11ShaderResourceView> Manager::CreateBlendMapFromDXT1(const uint8_t* dxtData, size_t dataSize, int width, int height)
    {
        if (!dxtData || dataSize == 0 || !mDevice) return nullptr;

        std::vector<uint8_t> rgba;
        if (!Tex::File::decodeDXT1(dxtData, width, height, rgba))
            return nullptr;

        for (size_t i = 0; i < rgba.size(); i += 4)
        {
            int r = rgba[i];
            int g = rgba[i + 1];
            int b = rgba[i + 2];
            int sum = r + g + b;
            rgba[i + 3] = static_cast<uint8_t>(std::max(0, 255 - sum));
        }

        return UploadRGBATexture(mDevice, mContext, rgba.data(), width, height, false);
    }

    ComPtr<ID3D11ShaderResourceView> Manager::CreateColorMapTexture(const uint8_t* data, int width, int height)
    {
        if (!data || width <= 0 || height <= 0 || !mDevice) return nullptr;
        return UploadRGBATexture(mDevice, mContext, data, width, height, false);
    }

    ComPtr<ID3D11ShaderResourceView> Manager::CreateColorMapFromDXT5(const uint8_t* dxtData, size_t dataSize, int width, int height)
    {
        if (!dxtData || dataSize == 0 || !mDevice) return nullptr;

        std::vector<uint8_t> rgba;
        if (!Tex::File::decodeDXT5(dxtData, width, height, rgba))
            return nullptr;

        return UploadRGBATexture(mDevice, mContext, rgba.data(), width, height, false);
    }

    ComPtr<ID3D11ShaderResourceView> UploadRGBATexture(ID3D11Device* device, ID3D11DeviceContext* context,
                                                       const uint8_t* data, int width, int height, bool generateMips)
    {
        if (!device || !data || width <= 0 || height <= 0) return nullptr;

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = generateMips ? 0 : 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0);
        texDesc.MiscFlags = generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

        ComPtr<ID3D11Texture2D> texture;

        if (generateMips)
        {
            HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &texture);
            if (FAILED(hr)) return nullptr;

            if (context)
            {
                context->UpdateSubresource(texture.Get(), 0, nullptr, data, width * 4, 0);
            }
        }
        else
        {
            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = data;
            initData.SysMemPitch = width * 4;

            HRESULT hr = device->CreateTexture2D(&texDesc, &initData, &texture);
            if (FAILED(hr)) return nullptr;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = generateMips ? static_cast<UINT>(-1) : 1;

        ComPtr<ID3D11ShaderResourceView> srv;
        HRESULT hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
        if (FAILED(hr)) return nullptr;

        if (generateMips && context)
        {
            context->GenerateMips(srv.Get());
        }

        return srv;
    }
}