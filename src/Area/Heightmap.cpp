#include "Heightmap.h"
#include "AreaFile.h"
#include "../UI/FileOps.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <d3d11.h>
#include <stb_image_write.h>
#include <mutex>
#include <atomic>
#include <future>
#include <cfloat>
#include <climits>
#include <algorithm>

extern ID3D11Device* gDevice;

namespace Heightmap {

    enum class PendingHeightmapType { None, Single, All };
    static PendingHeightmapType sPendingHeightmapType = PendingHeightmapType::None;
    static ArchivePtr sPendingHeightmapArchive;
    static std::shared_ptr<FileEntry> sPendingHeightmapEntry;
    static std::string sPendingHeightmapName;

    static std::vector<uint8_t> sCompositeHeightmapCache;
    static int sCompositeCacheWidth = 0;
    static int sCompositeCacheHeight = 0;
    static uint64_t sCompositeCacheGeneration = 0;
    static std::mutex sCompositeCacheMutex;

    static std::atomic<bool> sCompositeGenerating{false};
    static std::atomic<int> sCompositeProgressCurrent{0};
    static std::atomic<int> sCompositeProgressTotal{0};

    static bool sHeightmapWaitingForGeneration = false;
    static std::string sHeightmapWaitingFolderName;

    static std::vector<uint8_t> sHeightmapPixels;
    static std::string sHeightmapExportName;

    static bool sHeightmapViewerOpen = false;
    static std::string sHeightmapTitle;
    static ID3D11ShaderResourceView* sHeightmapSRV = nullptr;
    static int sHeightmapWidth = 0;
    static int sHeightmapHeight = 0;

    bool IsCompositeHeightmapGenerating()
    {
        return sCompositeGenerating;
    }

    float GetCompositeHeightmapProgress()
    {
        int total = sCompositeProgressTotal;
        if (total <= 0) return 0.0f;
        return static_cast<float>(sCompositeProgressCurrent) / static_cast<float>(total);
    }

    bool IsCompositeHeightmapReady(uint64_t currentGeneration)
    {
        std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
        return !sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == currentGeneration;
    }

    void ClearCompositeHeightmapCache()
    {
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            sCompositeHeightmapCache.clear();
            sCompositeCacheWidth = 0;
            sCompositeCacheHeight = 0;
        }
        sCompositeGenerating = false;
        sCompositeProgressCurrent = 0;
        sCompositeProgressTotal = 0;
        sHeightmapWaitingForGeneration = false;
    }

    bool GenerateCompositeThumbnailFromFiles(
        const std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>>& areaFiles,
        std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight,
        uint64_t generation)
    {
        if (areaFiles.empty()) return false;

        sCompositeGenerating = true;
        sCompositeProgressTotal = static_cast<int>(areaFiles.size());
        sCompositeProgressCurrent = 0;

        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
        numThreads = std::min(numThreads, 8u);
        numThreads = std::min(numThreads, static_cast<unsigned int>(areaFiles.size()));

        std::vector<ParsedArea> parsedAreas;
        parsedAreas.reserve(areaFiles.size());
        std::mutex resultsMutex;

        size_t totalFiles = areaFiles.size();
        size_t processed = 0;

        while (processed < totalFiles)
        {
            std::vector<std::future<ParsedArea>> futures;
            size_t batchEnd = std::min(processed + numThreads, totalFiles);

            for (size_t i = processed; i < batchEnd; i++)
            {
                const auto& [arc, fe] = areaFiles[i];
                futures.push_back(std::async(std::launch::async, [arc, fe]() {
                    return AreaFile::parseAreaFile(arc, fe);
                }));
            }

            for (auto& future : futures)
            {
                auto parsed = future.get();
                if (parsed.valid)
                {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    parsedAreas.push_back(std::move(parsed));
                }
                sCompositeProgressCurrent++;
            }

            processed = batchEnd;
        }

        int minTX = INT_MAX, maxTX = INT_MIN;
        int minTY = INT_MAX, maxTY = INT_MIN;
        float globalMinH = FLT_MAX, globalMaxH = -FLT_MAX;

        for (const auto& parsed : parsedAreas)
        {
            minTX = std::min(minTX, parsed.tileX);
            maxTX = std::max(maxTX, parsed.tileX);
            minTY = std::min(minTY, parsed.tileY);
            maxTY = std::max(maxTY, parsed.tileY);
            globalMinH = std::min(globalMinH, parsed.minBounds.y);
            globalMaxH = std::max(globalMaxH, parsed.maxHeight);
        }

        if (parsedAreas.empty())
        {
            sCompositeGenerating = false;
            return false;
        }

        int tilesX = maxTY - minTY + 1;
        int tilesY = maxTX - minTX + 1;
        int tilePixels = 256;
        int imgWidth = tilesX * tilePixels;
        int imgHeight = tilesY * tilePixels;

        std::vector<float> heights(imgWidth * imgHeight, globalMinH);

        for (const auto& parsed : parsedAreas)
        {
            int tileOffX = (parsed.tileY - minTY) * tilePixels;
            int tileOffY = (parsed.tileX - minTX) * tilePixels;

            for (int cz = 0; cz < 16; cz++)
            {
                for (int cx = 0; cx < 16; cx++)
                {
                    int chunkIdx = cz * 16 + cx;
                    if (chunkIdx >= (int)parsed.chunks.size()) continue;

                    const auto& chunk = parsed.chunks[chunkIdx];
                    if (!chunk.valid || chunk.vertices.empty()) continue;

                    for (int lz = 0; lz < 16; lz++)
                    {
                        for (int lx = 0; lx < 16; lx++)
                        {
                            int vIdx = lz * 17 + lx;
                            if (vIdx >= (int)chunk.vertices.size()) continue;

                            float height = chunk.vertices[vIdx].y;
                            int px = tileOffX + cx * 16 + lx;
                            int py = tileOffY + cz * 16 + lz;
                            if (px >= 0 && px < imgWidth && py >= 0 && py < imgHeight)
                            {
                                heights[py * imgWidth + px] = height;
                            }
                        }
                    }
                }
            }
        }

        int minPX = imgWidth, maxPX = 0;
        int minPY = imgHeight, maxPY = 0;

        for (int y = 0; y < imgHeight; y++)
        {
            for (int x = 0; x < imgWidth; x++)
            {
                if (heights[y * imgWidth + x] != globalMinH)
                {
                    minPX = std::min(minPX, x);
                    maxPX = std::max(maxPX, x);
                    minPY = std::min(minPY, y);
                    maxPY = std::max(maxPY, y);
                }
            }
        }

        if (minPX > maxPX) return false;

        int croppedWidth = maxPX - minPX + 1;
        int croppedHeight = maxPY - minPY + 1;

        float range = globalMaxH - globalMinH;
        if (range < 0.001f) range = 1.0f;

        std::vector<uint8_t> fullResPixels(croppedWidth * croppedHeight * 4);
        for (int y = 0; y < croppedHeight; y++)
        {
            for (int x = 0; x < croppedWidth; x++)
            {
                float h = heights[(y + minPY) * imgWidth + (x + minPX)];
                float norm = (h - globalMinH) / range;
                uint8_t val = (uint8_t)(std::clamp(norm, 0.0f, 1.0f) * 255.0f);

                int idx = (y * croppedWidth + x) * 4;
                fullResPixels[idx + 0] = val;
                fullResPixels[idx + 1] = val;
                fullResPixels[idx + 2] = val;
                fullResPixels[idx + 3] = 255;
            }
        }

        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            sCompositeHeightmapCache = fullResPixels;
            sCompositeCacheWidth = croppedWidth;
            sCompositeCacheHeight = croppedHeight;
            sCompositeCacheGeneration = generation;
        }

        sCompositeGenerating = false;

        int thumbSize = 256;
        float scale = std::min((float)thumbSize / croppedWidth, (float)thumbSize / croppedHeight);
        outWidth = std::max(1, (int)(croppedWidth * scale));
        outHeight = std::max(1, (int)(croppedHeight * scale));

        outPixels.resize(outWidth * outHeight * 4);
        for (int y = 0; y < outHeight; y++)
        {
            for (int x = 0; x < outWidth; x++)
            {
                int srcX = (int)(x / scale);
                int srcY = (int)(y / scale);
                srcX = std::min(srcX, croppedWidth - 1);
                srcY = std::min(srcY, croppedHeight - 1);

                int srcIdx = (srcY * croppedWidth + srcX) * 4;
                int dstIdx = (y * outWidth + x) * 4;
                outPixels[dstIdx + 0] = fullResPixels[srcIdx + 0];
                outPixels[dstIdx + 1] = fullResPixels[srcIdx + 1];
                outPixels[dstIdx + 2] = fullResPixels[srcIdx + 2];
                outPixels[dstIdx + 3] = 255;
            }
        }

        return true;
    }

    static void CreateHeightmapTextureOnly(const std::vector<float>& heights, int width, int height, float minH, float maxH)
    {
        if (sHeightmapSRV)
        {
            sHeightmapSRV->Release();
            sHeightmapSRV = nullptr;
        }

        if (heights.empty()) return;

        sHeightmapPixels.resize(width * height * 4);
        float range = (maxH - minH);
        if (range < 0.001f) range = 1.0f;

        for (int i = 0; i < width * height; i++)
        {
            float normalized = (heights[i] - minH) / range;
            uint8_t val = static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));
            sHeightmapPixels[i * 4 + 0] = val;
            sHeightmapPixels[i * 4 + 1] = val;
            sHeightmapPixels[i * 4 + 2] = val;
            sHeightmapPixels[i * 4 + 3] = 255;
        }

        sHeightmapWidth = width;
        sHeightmapHeight = height;
    }

    static void CreateHeightmapTexture(const std::vector<float>& heights, int width, int height, float minH, float maxH)
    {
        if (sHeightmapSRV)
        {
            sHeightmapSRV->Release();
            sHeightmapSRV = nullptr;
        }

        if (!gDevice || heights.empty()) return;

        sHeightmapPixels.resize(width * height * 4);
        float range = (maxH - minH);
        if (range < 0.001f) range = 1.0f;

        for (int i = 0; i < width * height; i++)
        {
            float normalized = (heights[i] - minH) / range;
            uint8_t val = static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));
            sHeightmapPixels[i * 4 + 0] = val;
            sHeightmapPixels[i * 4 + 1] = val;
            sHeightmapPixels[i * 4 + 2] = val;
            sHeightmapPixels[i * 4 + 3] = 255;
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = sHeightmapPixels.data();
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* tex = nullptr;
        if (SUCCEEDED(gDevice->CreateTexture2D(&desc, &initData, &tex)))
        {
            gDevice->CreateShaderResourceView(tex, nullptr, &sHeightmapSRV);
            tex->Release();
        }

        sHeightmapWidth = width;
        sHeightmapHeight = height;
    }

    void ExportHeightmapToFile(const std::string& path, const std::string& format)
    {
        if (sHeightmapPixels.empty() || sHeightmapWidth <= 0 || sHeightmapHeight <= 0) return;

        if (format == "png")
        {
            stbi_write_png(path.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapPixels.data(), sHeightmapWidth * 4);
        }
        else if (format == "jpg" || format == "jpeg")
        {
            stbi_write_jpg(path.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapPixels.data(), 95);
        }
        else if (format == "bmp")
        {
            stbi_write_bmp(path.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapPixels.data());
        }
        else if (format == "tga")
        {
            stbi_write_tga(path.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapPixels.data());
        }
    }

    void SetPendingSingleHeightmapExport(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry, const std::string& name)
    {
        sPendingHeightmapType = PendingHeightmapType::Single;
        sPendingHeightmapArchive = arc;
        sPendingHeightmapEntry = fileEntry;
        sPendingHeightmapName = name;
    }

    void SetPendingAllHeightmapExport(const std::string& folderName)
    {
        sPendingHeightmapType = PendingHeightmapType::All;
        sPendingHeightmapArchive = nullptr;
        sPendingHeightmapEntry = nullptr;
        sPendingHeightmapName = folderName;
    }

    void GenerateSingleAreaHeightmap(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry, const std::string& name)
    {
        if (!arc || !fileEntry) return;

        auto parsed = AreaFile::parseAreaFile(arc, fileEntry);
        if (!parsed.valid) return;

        int w = 256;
        int h = 256;
        std::vector<float> heights(w * h, 0.0f);

        float minH = parsed.minBounds.y;
        float maxH = parsed.maxHeight;

        for (int cz = 0; cz < 16; cz++)
        {
            for (int cx = 0; cx < 16; cx++)
            {
                int chunkIdx = cz * 16 + cx;
                if (chunkIdx >= (int)parsed.chunks.size()) continue;

                const auto& chunk = parsed.chunks[chunkIdx];
                if (!chunk.valid || chunk.vertices.empty()) continue;

                for (int lz = 0; lz < 16; lz++)
                {
                    for (int lx = 0; lx < 16; lx++)
                    {
                        int vIdx = lz * 17 + lx;
                        if (vIdx >= (int)chunk.vertices.size()) continue;

                        float height = chunk.vertices[vIdx].y;
                        int px = cx * 16 + lx;
                        int py = cz * 16 + lz;
                        heights[py * w + px] = height;
                    }
                }
            }
        }

        CreateHeightmapTextureOnly(heights, w, h, minH, maxH);
        sHeightmapExportName = name;
    }

    void ViewSingleAreaHeightmap(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry, const std::string& name)
    {
        if (!arc || !fileEntry) return;

        auto parsed = AreaFile::parseAreaFile(arc, fileEntry);
        if (!parsed.valid) return;

        int w = 256;
        int h = 256;
        std::vector<float> heights(w * h, 0.0f);

        float minH = parsed.minBounds.y;
        float maxH = parsed.maxHeight;

        for (int cz = 0; cz < 16; cz++)
        {
            for (int cx = 0; cx < 16; cx++)
            {
                int chunkIdx = cz * 16 + cx;
                if (chunkIdx >= (int)parsed.chunks.size()) continue;

                const auto& chunk = parsed.chunks[chunkIdx];
                if (!chunk.valid || chunk.vertices.empty()) continue;

                for (int lz = 0; lz < 16; lz++)
                {
                    for (int lx = 0; lx < 16; lx++)
                    {
                        int vIdx = lz * 17 + lx;
                        if (vIdx >= (int)chunk.vertices.size()) continue;

                        float height = chunk.vertices[vIdx].y;
                        int px = cx * 16 + lx;
                        int py = cz * 16 + lz;
                        heights[py * w + px] = height;
                    }
                }
            }
        }

        CreateHeightmapTexture(heights, w, h, minH, maxH);
        sHeightmapTitle = "Heightmap: " + name;
        sHeightmapExportName = name;
        sHeightmapViewerOpen = true;
    }

    void GenerateAllAreasHeightmap(const std::string& folderName, uint64_t currentGeneration)
    {
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            if (!sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == currentGeneration)
            {
                sHeightmapPixels = sCompositeHeightmapCache;
                sHeightmapWidth = sCompositeCacheWidth;
                sHeightmapHeight = sCompositeCacheHeight;
                sHeightmapExportName = folderName;
                return;
            }
        }

        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> areaFiles;
        for (const auto& file : UI_ContentBrowser::sCachedFiles)
        {
            if (!file.isDirectory && file.extension == ".area" && !file.isLoadAllEntry)
            {
                auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                if (fe)
                    areaFiles.push_back({file.archive, fe});
            }
        }

        if (areaFiles.empty()) return;

        std::vector<ParsedArea> parsedAreas;
        int minTX = INT_MAX, maxTX = INT_MIN;
        int minTY = INT_MAX, maxTY = INT_MIN;
        float globalMinH = FLT_MAX, globalMaxH = -FLT_MAX;

        for (const auto& [arc, fe] : areaFiles)
        {
            auto parsed = AreaFile::parseAreaFile(arc, fe);
            if (parsed.valid)
            {
                minTX = std::min(minTX, parsed.tileX);
                maxTX = std::max(maxTX, parsed.tileX);
                minTY = std::min(minTY, parsed.tileY);
                maxTY = std::max(maxTY, parsed.tileY);
                globalMinH = std::min(globalMinH, parsed.minBounds.y);
                globalMaxH = std::max(globalMaxH, parsed.maxHeight);
                parsedAreas.push_back(std::move(parsed));
            }
        }

        if (parsedAreas.empty()) return;

        int tilesX = maxTY - minTY + 1;
        int tilesY = maxTX - minTX + 1;
        int tilePixels = 256;
        int imgWidth = tilesX * tilePixels;
        int imgHeight = tilesY * tilePixels;

        std::vector<float> heights(imgWidth * imgHeight, globalMinH);

        for (const auto& parsed : parsedAreas)
        {
            int tileOffX = (parsed.tileY - minTY) * tilePixels;
            int tileOffY = (parsed.tileX - minTX) * tilePixels;

            for (int cz = 0; cz < 16; cz++)
            {
                for (int cx = 0; cx < 16; cx++)
                {
                    int chunkIdx = cz * 16 + cx;
                    if (chunkIdx >= (int)parsed.chunks.size()) continue;

                    const auto& chunk = parsed.chunks[chunkIdx];
                    if (!chunk.valid || chunk.vertices.empty()) continue;

                    for (int lz = 0; lz < 16; lz++)
                    {
                        for (int lx = 0; lx < 16; lx++)
                        {
                            int vIdx = lz * 17 + lx;
                            if (vIdx >= (int)chunk.vertices.size()) continue;

                            float height = chunk.vertices[vIdx].y;
                            int px = tileOffX + cx * 16 + lx;
                            int py = tileOffY + cz * 16 + lz;
                            if (px >= 0 && px < imgWidth && py >= 0 && py < imgHeight)
                            {
                                heights[py * imgWidth + px] = height;
                            }
                        }
                    }
                }
            }
        }

        int minPX = imgWidth, maxPX = 0;
        int minPY = imgHeight, maxPY = 0;
        bool hasData = false;

        for (int y = 0; y < imgHeight; y++)
        {
            for (int x = 0; x < imgWidth; x++)
            {
                if (heights[y * imgWidth + x] != globalMinH)
                {
                    minPX = std::min(minPX, x);
                    maxPX = std::max(maxPX, x);
                    minPY = std::min(minPY, y);
                    maxPY = std::max(maxPY, y);
                    hasData = true;
                }
            }
        }

        if (!hasData) return;

        int croppedWidth = maxPX - minPX + 1;
        int croppedHeight = maxPY - minPY + 1;
        std::vector<float> croppedHeights(croppedWidth * croppedHeight);

        for (int y = 0; y < croppedHeight; y++)
        {
            for (int x = 0; x < croppedWidth; x++)
            {
                croppedHeights[y * croppedWidth + x] = heights[(y + minPY) * imgWidth + (x + minPX)];
            }
        }

        CreateHeightmapTextureOnly(croppedHeights, croppedWidth, croppedHeight, globalMinH, globalMaxH);
        sHeightmapExportName = folderName;
    }

    void ViewAllAreasHeightmap(AppState& state, const std::string& folderName, uint64_t currentGeneration)
    {
        int areaCount = 0;
        for (const auto& file : UI_ContentBrowser::sCachedFiles)
        {
            if (!file.isDirectory && file.extension == ".area" && !file.isLoadAllEntry)
                areaCount++;
        }

        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            if (!sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == currentGeneration)
            {
                sHeightmapPixels = sCompositeHeightmapCache;
                sHeightmapWidth = sCompositeCacheWidth;
                sHeightmapHeight = sCompositeCacheHeight;

                if (sHeightmapSRV)
                {
                    sHeightmapSRV->Release();
                    sHeightmapSRV = nullptr;
                }

                if (gDevice && !sHeightmapPixels.empty())
                {
                    D3D11_TEXTURE2D_DESC desc = {};
                    desc.Width = sHeightmapWidth;
                    desc.Height = sHeightmapHeight;
                    desc.MipLevels = 1;
                    desc.ArraySize = 1;
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    desc.SampleDesc.Count = 1;
                    desc.Usage = D3D11_USAGE_DEFAULT;
                    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                    D3D11_SUBRESOURCE_DATA initData = {};
                    initData.pSysMem = sHeightmapPixels.data();
                    initData.SysMemPitch = sHeightmapWidth * 4;

                    ID3D11Texture2D* tex = nullptr;
                    if (SUCCEEDED(gDevice->CreateTexture2D(&desc, &initData, &tex)))
                    {
                        gDevice->CreateShaderResourceView(tex, nullptr, &sHeightmapSRV);
                        tex->Release();
                    }
                }

                sHeightmapTitle = "Heightmap: " + folderName + " (" + std::to_string(areaCount) + " areas)";
                sHeightmapExportName = folderName;
                sHeightmapWaitingForGeneration = false;
                sHeightmapViewerOpen = true;
                return;
            }
        }

        if (sCompositeGenerating)
        {
            sHeightmapTitle = "Heightmap: " + folderName + " (" + std::to_string(areaCount) + " areas)";
            sHeightmapWaitingFolderName = folderName;
            sHeightmapWaitingForGeneration = true;
            sHeightmapViewerOpen = true;
            return;
        }

        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> areaFiles;
        for (const auto& file : UI_ContentBrowser::sCachedFiles)
        {
            if (!file.isDirectory && file.extension == ".area" && !file.isLoadAllEntry)
            {
                auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                if (fe)
                    areaFiles.push_back({file.archive, fe});
            }
        }

        if (areaFiles.empty()) return;

        std::vector<ParsedArea> parsedAreas;
        int minTX = INT_MAX, maxTX = INT_MIN;
        int minTY = INT_MAX, maxTY = INT_MIN;
        float globalMinH = FLT_MAX, globalMaxH = -FLT_MAX;

        for (const auto& [arc, fe] : areaFiles)
        {
            auto parsed = AreaFile::parseAreaFile(arc, fe);
            if (parsed.valid)
            {
                minTX = std::min(minTX, parsed.tileX);
                maxTX = std::max(maxTX, parsed.tileX);
                minTY = std::min(minTY, parsed.tileY);
                maxTY = std::max(maxTY, parsed.tileY);
                globalMinH = std::min(globalMinH, parsed.minBounds.y);
                globalMaxH = std::max(globalMaxH, parsed.maxHeight);
                parsedAreas.push_back(std::move(parsed));
            }
        }

        if (parsedAreas.empty()) return;

        int tilesX = maxTY - minTY + 1;
        int tilesY = maxTX - minTX + 1;
        int tilePixels = 256;
        int imgWidth = tilesX * tilePixels;
        int imgHeight = tilesY * tilePixels;

        std::vector<float> heights(imgWidth * imgHeight, globalMinH);

        for (const auto& parsed : parsedAreas)
        {
            int tileOffX = (parsed.tileY - minTY) * tilePixels;
            int tileOffY = (parsed.tileX - minTX) * tilePixels;

            for (int cz = 0; cz < 16; cz++)
            {
                for (int cx = 0; cx < 16; cx++)
                {
                    int chunkIdx = cz * 16 + cx;
                    if (chunkIdx >= (int)parsed.chunks.size()) continue;

                    const auto& chunk = parsed.chunks[chunkIdx];
                    if (!chunk.valid || chunk.vertices.empty()) continue;

                    for (int lz = 0; lz < 16; lz++)
                    {
                        for (int lx = 0; lx < 16; lx++)
                        {
                            int vIdx = lz * 17 + lx;
                            if (vIdx >= (int)chunk.vertices.size()) continue;

                            float height = chunk.vertices[vIdx].y;
                            int px = tileOffX + cx * 16 + lx;
                            int py = tileOffY + cz * 16 + lz;
                            if (px >= 0 && px < imgWidth && py >= 0 && py < imgHeight)
                            {
                                heights[py * imgWidth + px] = height;
                            }
                        }
                    }
                }
            }
        }

        int minPX = imgWidth, maxPX = 0;
        int minPY = imgHeight, maxPY = 0;
        bool hasData = false;

        for (int y = 0; y < imgHeight; y++)
        {
            for (int x = 0; x < imgWidth; x++)
            {
                if (heights[y * imgWidth + x] != globalMinH)
                {
                    minPX = std::min(minPX, x);
                    maxPX = std::max(maxPX, x);
                    minPY = std::min(minPY, y);
                    maxPY = std::max(maxPY, y);
                    hasData = true;
                }
            }
        }

        if (!hasData) return;

        int croppedWidth = maxPX - minPX + 1;
        int croppedHeight = maxPY - minPY + 1;
        std::vector<float> croppedHeights(croppedWidth * croppedHeight);

        for (int y = 0; y < croppedHeight; y++)
        {
            for (int x = 0; x < croppedWidth; x++)
            {
                croppedHeights[y * croppedWidth + x] = heights[(y + minPY) * imgWidth + (x + minPX)];
            }
        }

        CreateHeightmapTexture(croppedHeights, croppedWidth, croppedHeight, globalMinH, globalMaxH);
        sHeightmapTitle = "Heightmap: " + folderName + " (" + std::to_string(parsedAreas.size()) + " areas)";
        sHeightmapExportName = folderName;
        sHeightmapViewerOpen = true;
    }

    void DrawHeightmapViewer(uint64_t currentGeneration)
    {
        if (ImGuiFileDialog::Instance()->Display("ExportHeightmapPngDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                if (sPendingHeightmapType == PendingHeightmapType::Single)
                    GenerateSingleAreaHeightmap(sPendingHeightmapArchive, sPendingHeightmapEntry, sPendingHeightmapName);
                else if (sPendingHeightmapType == PendingHeightmapType::All)
                    GenerateAllAreasHeightmap(sPendingHeightmapName, currentGeneration);
                sPendingHeightmapType = PendingHeightmapType::None;

                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                ExportHeightmapToFile(path, "png");
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportHeightmapJpgDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                if (sPendingHeightmapType == PendingHeightmapType::Single)
                    GenerateSingleAreaHeightmap(sPendingHeightmapArchive, sPendingHeightmapEntry, sPendingHeightmapName);
                else if (sPendingHeightmapType == PendingHeightmapType::All)
                    GenerateAllAreasHeightmap(sPendingHeightmapName, currentGeneration);
                sPendingHeightmapType = PendingHeightmapType::None;

                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                ExportHeightmapToFile(path, "jpg");
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportHeightmapBmpDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                if (sPendingHeightmapType == PendingHeightmapType::Single)
                    GenerateSingleAreaHeightmap(sPendingHeightmapArchive, sPendingHeightmapEntry, sPendingHeightmapName);
                else if (sPendingHeightmapType == PendingHeightmapType::All)
                    GenerateAllAreasHeightmap(sPendingHeightmapName, currentGeneration);
                sPendingHeightmapType = PendingHeightmapType::None;

                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                ExportHeightmapToFile(path, "bmp");
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportHeightmapTgaDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                if (sPendingHeightmapType == PendingHeightmapType::Single)
                    GenerateSingleAreaHeightmap(sPendingHeightmapArchive, sPendingHeightmapEntry, sPendingHeightmapName);
                else if (sPendingHeightmapType == PendingHeightmapType::All)
                    GenerateAllAreasHeightmap(sPendingHeightmapName, currentGeneration);
                sPendingHeightmapType = PendingHeightmapType::None;

                std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
                ExportHeightmapToFile(path, "tga");
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (!sHeightmapViewerOpen)
        {
            if (sHeightmapSRV)
            {
                sHeightmapSRV->Release();
                sHeightmapSRV = nullptr;
                sHeightmapPixels.clear();
            }
            return;
        }

        if (sHeightmapWaitingForGeneration)
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            if (!sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == currentGeneration)
            {
                sHeightmapPixels = sCompositeHeightmapCache;
                sHeightmapWidth = sCompositeCacheWidth;
                sHeightmapHeight = sCompositeCacheHeight;

                if (sHeightmapSRV)
                {
                    sHeightmapSRV->Release();
                    sHeightmapSRV = nullptr;
                }

                if (gDevice && !sHeightmapPixels.empty())
                {
                    D3D11_TEXTURE2D_DESC desc = {};
                    desc.Width = sHeightmapWidth;
                    desc.Height = sHeightmapHeight;
                    desc.MipLevels = 1;
                    desc.ArraySize = 1;
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    desc.SampleDesc.Count = 1;
                    desc.Usage = D3D11_USAGE_DEFAULT;
                    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                    D3D11_SUBRESOURCE_DATA initData = {};
                    initData.pSysMem = sHeightmapPixels.data();
                    initData.SysMemPitch = sHeightmapWidth * 4;

                    ID3D11Texture2D* tex = nullptr;
                    if (SUCCEEDED(gDevice->CreateTexture2D(&desc, &initData, &tex)))
                    {
                        gDevice->CreateShaderResourceView(tex, nullptr, &sHeightmapSRV);
                        tex->Release();
                    }
                }

                sHeightmapExportName = sHeightmapWaitingFolderName;
                sHeightmapWaitingForGeneration = false;
            }
        }

        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(sHeightmapTitle.c_str(), &sHeightmapViewerOpen))
        {
            if (sHeightmapWaitingForGeneration)
            {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + avail.x * 0.1f, ImGui::GetCursorPosY() + avail.y * 0.45f));

                float progress = GetCompositeHeightmapProgress();
                int current = sCompositeProgressCurrent;
                int total = sCompositeProgressTotal;

                char overlay[64];
                snprintf(overlay, sizeof(overlay), "Loading areas... %d / %d", current, total);

                ImGui::ProgressBar(progress, ImVec2(avail.x * 0.8f, 0), overlay);
            }
            else if (sHeightmapSRV)
            {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                float aspectImg = (float)sHeightmapWidth / (float)sHeightmapHeight;
                float aspectWin = avail.x / avail.y;

                float displayW, displayH;
                if (aspectImg > aspectWin)
                {
                    displayW = avail.x;
                    displayH = avail.x / aspectImg;
                }
                else
                {
                    displayH = avail.y;
                    displayW = avail.y * aspectImg;
                }

                ImVec2 cursorPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(cursorPos.x + (avail.x - displayW) * 0.5f, cursorPos.y + (avail.y - displayH) * 0.5f));
                ImGui::Image((ImTextureID)sHeightmapSRV, ImVec2(displayW, displayH));

                if (ImGui::BeginPopupContextItem("##heightmapexport"))
                {
                    if (ImGui::BeginMenu("Extract Heightmap"))
                    {
                        if (ImGui::MenuItem("PNG"))
                        {
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            config.fileName = sHeightmapExportName + "_heightmap.png";
                            config.flags = ImGuiFileDialogFlags_Modal;
                            ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapPngDlg", "Export Heightmap as PNG", ".png", config);
                        }
                        if (ImGui::MenuItem("JPEG"))
                        {
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            config.fileName = sHeightmapExportName + "_heightmap.jpg";
                            config.flags = ImGuiFileDialogFlags_Modal;
                            ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapJpgDlg", "Export Heightmap as JPEG", ".jpg", config);
                        }
                        if (ImGui::MenuItem("BMP"))
                        {
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            config.fileName = sHeightmapExportName + "_heightmap.bmp";
                            config.flags = ImGuiFileDialogFlags_Modal;
                            ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapBmpDlg", "Export Heightmap as BMP", ".bmp", config);
                        }
                        if (ImGui::MenuItem("TGA"))
                        {
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            config.fileName = sHeightmapExportName + "_heightmap.tga";
                            config.flags = ImGuiFileDialogFlags_Modal;
                            ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapTgaDlg", "Export Heightmap as TGA", ".tga", config);
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndPopup();
                }
            }
            else
            {
                ImGui::Text("No heightmap data");
            }
        }
        ImGui::End();
    }

}