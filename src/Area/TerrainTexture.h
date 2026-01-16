#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glad/glad.h>

class Archive;
using ArchivePtr = std::shared_ptr<Archive>;

namespace TerrainTexture
{
    struct CachedTexture
    {
        GLuint diffuse = 0;
        GLuint normal = 0;
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

        bool LoadWorldLayerTable(const ArchivePtr& archive);
        const CachedTexture* GetLayerTexture(const ArchivePtr& archive, uint32_t layerId);
        const WorldLayerEntry* GetLayerEntry(uint32_t layerId) const;

        bool GetLayerTextureData(const ArchivePtr& archive, uint32_t layerId, RawTextureData& outData);

        bool LoadRawTextureFromPath(const ArchivePtr& archive, const std::string& path, RawTextureData& outData);

        GLuint CreateBlendMapTexture(const uint8_t* data, int width, int height);
        GLuint CreateBlendMapFromDXT1(const uint8_t* dxtData, size_t dataSize, int width, int height);
        GLuint CreateColorMapTexture(const uint8_t* data, int width, int height);
        GLuint CreateColorMapFromDXT5(const uint8_t* dxtData, size_t dataSize, int width, int height);

        void ClearCache();
        bool IsTableLoaded() const { return mTableLoaded; }

    private:
        Manager() = default;
        ~Manager();

        bool LoadTextureFromPath(const ArchivePtr& archive, const std::string& path, GLuint& outTexture, int& outW, int& outH);

        std::unordered_map<uint32_t, WorldLayerEntry> mLayerTable;
        std::unordered_map<uint32_t, CachedTexture> mTextureCache;

        struct PathCachedTexture
        {
            GLuint texture = 0;
            int width = 0;
            int height = 0;
        };
        std::unordered_map<std::string, PathCachedTexture> mPathTextureCache;

        GLuint mFallbackWhite = 0;
        GLuint mFallbackNormal = 0;
        void EnsureFallbackTextures();

        bool mTableLoaded = false;
    };

    GLuint UploadRGBATexture(const uint8_t* data, int width, int height, bool generateMips = true);
}