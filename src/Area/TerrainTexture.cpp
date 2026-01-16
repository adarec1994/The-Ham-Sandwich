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

    void Manager::ClearCache()
    {
        for (auto& [path, tex] : mPathTextureCache)
        {
            if (tex.texture != 0) glDeleteTextures(1, &tex.texture);
        }
        mPathTextureCache.clear();
        mTextureCache.clear();

        if (mFallbackWhite != 0)
        {
            glDeleteTextures(1, &mFallbackWhite);
            mFallbackWhite = 0;
        }
        if (mFallbackNormal != 0)
        {
            glDeleteTextures(1, &mFallbackNormal);
            mFallbackNormal = 0;
        }
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
            // Try recursive filename search as last resort
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

    bool Manager::LoadTextureFromPath(const ArchivePtr& archive, const std::string& path, GLuint& outTexture, int& outW, int& outH)
    {
        if (!archive || path.empty()) return false;

        auto cacheIt = mPathTextureCache.find(path);
        if (cacheIt != mPathTextureCache.end())
        {
            outTexture = cacheIt->second.texture;
            outW = cacheIt->second.width;
            outH = cacheIt->second.height;
            return outTexture != 0;
        }

        RawTextureData rawData;
        if (!LoadRawTextureFromPath(archive, path, rawData))
        {
            mPathTextureCache[path] = {0, 0, 0};
            return false;
        }

        outTexture = UploadRGBATexture(rawData.rgba.data(), rawData.width, rawData.height, true);
        outW = rawData.width;
        outH = rawData.height;

        mPathTextureCache[path] = {outTexture, outW, outH};

        return outTexture != 0;
    }

    void Manager::EnsureFallbackTextures()
    {
        if (mFallbackWhite == 0)
        {
            uint8_t white[4] = {255, 255, 255, 255};
            mFallbackWhite = UploadRGBATexture(white, 1, 1, false);
        }
        if (mFallbackNormal == 0)
        {
            uint8_t normal[4] = {128, 128, 255, 255};
            mFallbackNormal = UploadRGBATexture(normal, 1, 1, false);
        }
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

        if (tex.diffuse == 0)
        {
            tex.diffuse = mFallbackWhite;
            tex.width = 1;
            tex.height = 1;
        }

        if (tex.normal == 0)
        {
            tex.normal = mFallbackNormal;
        }

        tex.loaded = true;
        mTextureCache[layerId] = tex;
        return &mTextureCache[layerId];
    }

    GLuint Manager::CreateBlendMapTexture(const uint8_t* data, int width, int height)
    {
        if (!data || width <= 0 || height <= 0) return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint Manager::CreateBlendMapFromDXT1(const uint8_t* dxtData, size_t dataSize, int width, int height)
    {
        if (!dxtData || dataSize == 0) return 0;

        std::vector<uint8_t> rgba;
        if (!Tex::File::decodeDXT1(dxtData, width, height, rgba))
            return 0;

        for (size_t i = 0; i < rgba.size(); i += 4)
        {
            int r = rgba[i];
            int g = rgba[i + 1];
            int b = rgba[i + 2];
            int sum = r + g + b;
            rgba[i + 3] = static_cast<uint8_t>(std::max(0, 255 - sum));
        }

        return CreateBlendMapTexture(rgba.data(), width, height);
    }

    GLuint Manager::CreateColorMapTexture(const uint8_t* data, int width, int height)
    {
        if (!data || width <= 0 || height <= 0) return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint Manager::CreateColorMapFromDXT5(const uint8_t* dxtData, size_t dataSize, int width, int height)
    {
        if (!dxtData || dataSize == 0) return 0;

        std::vector<uint8_t> rgba;
        if (!Tex::File::decodeDXT5(dxtData, width, height, rgba))
            return 0;

        return CreateColorMapTexture(rgba.data(), width, height);
    }

    GLuint UploadRGBATexture(const uint8_t* data, int width, int height, bool generateMips)
    {
        if (!data || width <= 0 || height <= 0) return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        if (generateMips)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        if (generateMips)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }
}