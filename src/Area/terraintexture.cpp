#include "TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include "../Database/Tbl.h"
#include "../Database/TblReader.h"
#include "../Database/Definitions/WorldLayer.h"
#include <iostream>
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
        for (auto& [id, tex] : mTextureCache)
        {
            if (tex.diffuse != 0) glDeleteTextures(1, &tex.diffuse);
            if (tex.normal != 0) glDeleteTextures(1, &tex.normal);
        }
        mTextureCache.clear();
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
            std::cerr << "WorldLayer.tbl not found in archive\n";
            mTableLoaded = true;
            return false;
        }

        std::vector<uint8_t> data;
        if (!archive->getFileData(fileEntry, data))
        {
            std::cerr << "Failed to read WorldLayer.tbl\n";
            mTableLoaded = true;
            return false;
        }

        Tbl::Table<Database::Definitions::WorldLayer> tbl;
        if (!tbl.load(data.data(), data.size()))
        {
            std::cerr << "Failed to parse WorldLayer.tbl\n";
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
        std::cout << "Loaded WorldLayer.tbl with " << mLayerTable.size() << " entries\n";
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

    bool Manager::LoadTextureFromPath(const ArchivePtr& archive, const std::string& path, GLuint& outTexture, int& outW, int& outH)
    {
        if (!archive || path.empty()) return false;

        std::wstring wpath(path.begin(), path.end());

        auto root = archive->getRoot();
        if (!root) return false;

        auto fileEntry = FindFileByPath(root, wpath);
        if (!fileEntry)
        {
            return false;
        }

        std::vector<uint8_t> bytes;
        if (!archive->getFileData(fileEntry, bytes))
            return false;

        if (bytes.empty()) return false;

        Tex::File tf;
        if (!tf.readFromMemory(bytes.data(), bytes.size()))
            return false;

        Tex::ImageRGBA img;
        if (!tf.decodeLargestMipToRGBA(img))
            return false;

        outTexture = UploadRGBATexture(img.rgba.data(), img.width, img.height, true);
        outW = img.width;
        outH = img.height;
        return outTexture != 0;
    }

    const CachedTexture* Manager::GetLayerTexture(const ArchivePtr& archive, uint32_t layerId)
    {
        auto cacheIt = mTextureCache.find(layerId);
        if (cacheIt != mTextureCache.end() && cacheIt->second.loaded)
            return &cacheIt->second;

        const WorldLayerEntry* entry = GetLayerEntry(layerId);
        if (!entry)
        {
            std::cerr << "WorldLayer entry not found for ID " << layerId << "\n";
            CachedTexture tex;
            uint8_t white[4] = {255, 255, 255, 255};
            tex.diffuse = UploadRGBATexture(white, 1, 1, false);
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
            uint8_t white[4] = {255, 255, 255, 255};
            tex.diffuse = UploadRGBATexture(white, 1, 1, false);
            tex.width = 1;
            tex.height = 1;
        }

        tex.loaded = true;
        mTextureCache[layerId] = tex;
        return &mTextureCache[layerId];
    }

    bool Manager::DecompressDXT1(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;

        outRGBA.assign(static_cast<size_t>(width) * height * 4, 0);

        int blocksX = (width + 3) / 4;
        int blocksY = (height + 3) / 4;
        const uint8_t* p = src;

        auto rgb565_to_rgb8 = [](uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
            r = static_cast<uint8_t>(((c >> 11) & 31) * 255 / 31);
            g = static_cast<uint8_t>(((c >> 5) & 63) * 255 / 63);
            b = static_cast<uint8_t>((c & 31) * 255 / 31);
        };

        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                uint16_t c0 = static_cast<uint16_t>(p[0] | (p[1] << 8));
                uint16_t c1 = static_cast<uint16_t>(p[2] | (p[3] << 8));
                uint32_t codes;
                std::memcpy(&codes, p + 4, 4);
                p += 8;

                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565_to_rgb8(c0, r0, g0, b0);
                rgb565_to_rgb8(c1, r1, g1, b1);

                uint8_t pal[4][4]{};
                pal[0][0] = r0; pal[0][1] = g0; pal[0][2] = b0; pal[0][3] = 255;
                pal[1][0] = r1; pal[1][1] = g1; pal[1][2] = b1; pal[1][3] = 255;

                if (c0 > c1)
                {
                    pal[2][0] = static_cast<uint8_t>((2 * r0 + r1) / 3);
                    pal[2][1] = static_cast<uint8_t>((2 * g0 + g1) / 3);
                    pal[2][2] = static_cast<uint8_t>((2 * b0 + b1) / 3);
                    pal[2][3] = 255;
                    pal[3][0] = static_cast<uint8_t>((r0 + 2 * r1) / 3);
                    pal[3][1] = static_cast<uint8_t>((g0 + 2 * g1) / 3);
                    pal[3][2] = static_cast<uint8_t>((b0 + 2 * b1) / 3);
                    pal[3][3] = 255;
                }
                else
                {
                    pal[2][0] = static_cast<uint8_t>((r0 + r1) / 2);
                    pal[2][1] = static_cast<uint8_t>((g0 + g1) / 2);
                    pal[2][2] = static_cast<uint8_t>((b0 + b1) / 2);
                    pal[2][3] = 255;
                    pal[3][0] = 0; pal[3][1] = 0; pal[3][2] = 0; pal[3][3] = 0;
                }

                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
                    {
                        int x = bx * 4 + px;
                        int y = by * 4 + py;
                        if (x >= width || y >= height) continue;
                        uint32_t idx = (codes >> (2 * (py * 4 + px))) & 0x3;
                        size_t o = (static_cast<size_t>(y) * width + x) * 4;
                        outRGBA[o + 0] = pal[idx][0];
                        outRGBA[o + 1] = pal[idx][1];
                        outRGBA[o + 2] = pal[idx][2];
                        outRGBA[o + 3] = pal[idx][3];
                    }
                }
            }
        }
        return true;
    }

    bool Manager::DecompressDXT5(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;

        outRGBA.assign(static_cast<size_t>(width) * height * 4, 0);

        int blocksX = (width + 3) / 4;
        int blocksY = (height + 3) / 4;
        const uint8_t* p = src;

        auto rgb565_to_rgb8 = [](uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
            r = static_cast<uint8_t>(((c >> 11) & 31) * 255 / 31);
            g = static_cast<uint8_t>(((c >> 5) & 63) * 255 / 63);
            b = static_cast<uint8_t>((c & 31) * 255 / 31);
        };

        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                uint8_t a0 = p[0];
                uint8_t a1 = p[1];

                uint64_t alphaBits = 0;
                for (int i = 0; i < 6; ++i)
                    alphaBits |= static_cast<uint64_t>(p[2 + i]) << (8 * i);

                uint8_t alphaPal[8];
                alphaPal[0] = a0;
                alphaPal[1] = a1;
                if (a0 > a1)
                {
                    alphaPal[2] = static_cast<uint8_t>((6 * a0 + 1 * a1) / 7);
                    alphaPal[3] = static_cast<uint8_t>((5 * a0 + 2 * a1) / 7);
                    alphaPal[4] = static_cast<uint8_t>((4 * a0 + 3 * a1) / 7);
                    alphaPal[5] = static_cast<uint8_t>((3 * a0 + 4 * a1) / 7);
                    alphaPal[6] = static_cast<uint8_t>((2 * a0 + 5 * a1) / 7);
                    alphaPal[7] = static_cast<uint8_t>((1 * a0 + 6 * a1) / 7);
                }
                else
                {
                    alphaPal[2] = static_cast<uint8_t>((4 * a0 + 1 * a1) / 5);
                    alphaPal[3] = static_cast<uint8_t>((3 * a0 + 2 * a1) / 5);
                    alphaPal[4] = static_cast<uint8_t>((2 * a0 + 3 * a1) / 5);
                    alphaPal[5] = static_cast<uint8_t>((1 * a0 + 4 * a1) / 5);
                    alphaPal[6] = 0;
                    alphaPal[7] = 255;
                }

                p += 8;

                uint16_t c0 = static_cast<uint16_t>(p[0] | (p[1] << 8));
                uint16_t c1 = static_cast<uint16_t>(p[2] | (p[3] << 8));
                uint32_t codes;
                std::memcpy(&codes, p + 4, 4);
                p += 8;

                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565_to_rgb8(c0, r0, g0, b0);
                rgb565_to_rgb8(c1, r1, g1, b1);

                uint8_t pal[4][3]{};
                pal[0][0] = r0; pal[0][1] = g0; pal[0][2] = b0;
                pal[1][0] = r1; pal[1][1] = g1; pal[1][2] = b1;
                pal[2][0] = static_cast<uint8_t>((2 * r0 + r1) / 3);
                pal[2][1] = static_cast<uint8_t>((2 * g0 + g1) / 3);
                pal[2][2] = static_cast<uint8_t>((2 * b0 + b1) / 3);
                pal[3][0] = static_cast<uint8_t>((r0 + 2 * r1) / 3);
                pal[3][1] = static_cast<uint8_t>((g0 + 2 * g1) / 3);
                pal[3][2] = static_cast<uint8_t>((b0 + 2 * b1) / 3);

                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
                    {
                        int x = bx * 4 + px;
                        int y = by * 4 + py;
                        if (x >= width || y >= height) continue;

                        int pixelIdx = py * 4 + px;
                        uint32_t colorIdx = (codes >> (2 * pixelIdx)) & 0x3;
                        uint32_t alphaIdx = (alphaBits >> (3 * pixelIdx)) & 0x7;

                        size_t o = (static_cast<size_t>(y) * width + x) * 4;
                        outRGBA[o + 0] = pal[colorIdx][0];
                        outRGBA[o + 1] = pal[colorIdx][1];
                        outRGBA[o + 2] = pal[colorIdx][2];
                        outRGBA[o + 3] = alphaPal[alphaIdx];
                    }
                }
            }
        }
        return true;
    }

    GLuint Manager::CreateBlendMapFromDXT1(const uint8_t* dxtData, size_t dataSize, int width, int height)
    {
        if (!dxtData || dataSize == 0) return 0;

        std::vector<uint8_t> rgba;
        if (!DecompressDXT1(dxtData, width, height, rgba))
            return 0;

        return UploadRGBATexture(rgba.data(), width, height, false);
    }

    GLuint Manager::CreateColorMapFromDXT5(const uint8_t* dxtData, size_t dataSize, int width, int height)
    {
        if (!dxtData || dataSize == 0) return 0;

        std::vector<uint8_t> rgba;
        if (!DecompressDXT5(dxtData, width, height, rgba))
            return 0;

        return UploadRGBATexture(rgba.data(), width, height, false);
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