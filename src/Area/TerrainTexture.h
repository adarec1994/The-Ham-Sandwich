#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Archive;
using ArchivePtr = std::shared_ptr<Archive>;

namespace TerrainTexture
{
    struct CachedTexture
    {
        ComPtr<ID3D11ShaderResourceView> diffuse;
        ComPtr<ID3D11ShaderResourceView> normal;
        int width = 0;
        int height = 0;
        bool loaded = false;
    };

    struct WorldLayerEntry
    {
        uint32_t id = 0;
        std::string diffusePath;
        std::string normalPath;
        float scaleU = 1.0f;
        float scaleV = 1.0f;
    };

    struct RawTextureData
    {
        std::vector<uint8_t> rgba;
        int width = 0;
        int height = 0;
        bool valid = false;
    };

    class Manager
    {
    public:
        static Manager& Instance();

        void SetDevice(ID3D11Device* device, ID3D11DeviceContext* context);

        bool LoadWorldLayerTable(const ArchivePtr& archive);
        const CachedTexture* GetLayerTexture(const ArchivePtr& archive, uint32_t layerId);
        const WorldLayerEntry* GetLayerEntry(uint32_t layerId) const;

        bool GetLayerTextureData(const ArchivePtr& archive, uint32_t layerId, RawTextureData& outData);

        bool LoadRawTextureFromPath(const ArchivePtr& archive, const std::string& path, RawTextureData& outData);

        ComPtr<ID3D11ShaderResourceView> CreateBlendMapTexture(const uint8_t* data, int width, int height);
        ComPtr<ID3D11ShaderResourceView> CreateBlendMapFromDXT1(const uint8_t* dxtData, size_t dataSize, int width, int height);
        ComPtr<ID3D11ShaderResourceView> CreateColorMapTexture(const uint8_t* data, int width, int height);
        ComPtr<ID3D11ShaderResourceView> CreateColorMapFromDXT5(const uint8_t* dxtData, size_t dataSize, int width, int height);

        void ClearCache();
        bool IsTableLoaded() const { return mTableLoaded; }

        ID3D11ShaderResourceView* GetFallbackWhite();
        ID3D11ShaderResourceView* GetFallbackNormal();

    private:
        Manager() = default;
        ~Manager();

        bool LoadTextureFromPath(const ArchivePtr& archive, const std::string& path,
                                 ComPtr<ID3D11ShaderResourceView>& outSRV, int& outW, int& outH);

        std::unordered_map<uint32_t, WorldLayerEntry> mLayerTable;
        std::unordered_map<uint32_t, CachedTexture> mTextureCache;

        struct PathCachedTexture
        {
            ComPtr<ID3D11ShaderResourceView> srv;
            int width = 0;
            int height = 0;
        };
        std::unordered_map<std::string, PathCachedTexture> mPathTextureCache;

        ComPtr<ID3D11ShaderResourceView> mFallbackWhite;
        ComPtr<ID3D11ShaderResourceView> mFallbackNormal;
        void EnsureFallbackTextures();

        bool mTableLoaded = false;

        ID3D11Device* mDevice = nullptr;
        ID3D11DeviceContext* mContext = nullptr;
    };

    ComPtr<ID3D11ShaderResourceView> UploadRGBATexture(ID3D11Device* device, ID3D11DeviceContext* context,
                                                       const uint8_t* data, int width, int height, bool generateMips = true);
}