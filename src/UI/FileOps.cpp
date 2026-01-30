#include "FileOps.h"
#include "UI.h"
#include "UI_Globals.h"
#include "UI_Utils.h"
#include "UI_Tables.h"
#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include "../tex/tex.h"
#include "../Audio/AudioPlayer.h"
#include "../Audio/AudioPlayerWidget.h"
#include "../BNK/BNK_hirc.h"
#include "../Skybox/Sky_Manager.h"
#include <ImGuiFileDialog.h>
#include <algorithm>
#include <filesystem>
#include <d3d11.h>
#include <cstring>
#include <cfloat>
#include <climits>
#include <stb_image_write.h>
#include <future>

extern void SnapCameraToLoaded(AppState& state);
extern ID3D11Device* gDevice;

namespace UI_ContentBrowser {

    bool sIsOpen = true;
    bool sIsDocked = true;
    float sCurrentHeight = 300.0f;
    float sTargetHeight = 300.0f;
    float sAnimSpeed = 1500.0f;
    float sTreeWidth = 200.0f;
    ImTextureID sFolderIcon = 0;
    ImTextureID sContentBrowserIcon = 0;
    ImTextureID sTblIcon = 0;
    ImTextureID sAreaIcon = 0;
    ImTextureID sAudioIcon = 0;

    IFileSystemEntryPtr sSelectedFolder = nullptr;
    ArchivePtr sSelectedArchive = nullptr;
    std::string sSelectedPath;

    char sSearchFilter[256] = "";

    std::vector<FileInfo> sCachedFiles;
    bool sNeedsRefresh = true;
    int sSelectedFileIndex = -1;

    bool sRequestTreeSync = false;
    std::unordered_set<const void*> sNodesToExpand;

    std::vector<IFileSystemEntryPtr> sBreadcrumbPath;

    ArchivePtr sExportArchive = nullptr;
    std::shared_ptr<FileEntry> sExportFileEntry = nullptr;
    std::string sExportDefaultName;
    std::atomic<bool> sExportInProgress{false};
    std::atomic<int> sExportProgress{0};
    std::atomic<int> sExportTotal{100};
    std::string sExportStatus;
    std::mutex sExportMutex;
    M3Export::ExportResult sExportResult;
    bool sShowExportResult = false;
    float sNotificationTimer = 0.0f;
    std::string sNotificationMessage;
    bool sNotificationSuccess = true;

    std::vector<uint8_t> sAudioExportData;
    std::string sAudioExportName;

    std::vector<uint8_t> sBnkExportData;
    std::string sBnkExportName;

    bool sBnkViewActive = false;
    std::vector<uint8_t> sBnkViewData;
    std::string sBnkViewName;
    ArchivePtr sBnkViewArchive;
    std::vector<BnkWemEntry> sBnkWemEntries;

    std::vector<uint8_t> sTblExportData;
    std::string sTblExportName;

    Bnk::WemNameResolver sWemResolver;

    std::deque<ThumbnailRequest> sLoadQueue;
    std::deque<ThumbnailResult> sResultQueue;
    std::mutex sQueueMutex;
    std::atomic<bool> sWorkerRunning{ false };
    std::thread sWorkerThread;
    uint64_t sCurrentGeneration = 0;
    std::condition_variable sQueueCV;

    // Pending heightmap export (deferred until save dialog confirms)
    enum class PendingHeightmapType { None, Single, All };
    static PendingHeightmapType sPendingHeightmapType = PendingHeightmapType::None;
    static ArchivePtr sPendingHeightmapArchive;
    static std::shared_ptr<FileEntry> sPendingHeightmapEntry;
    static std::string sPendingHeightmapName;

    // Cached composite heightmap (full resolution, from background generation)
    static std::vector<uint8_t> sCompositeHeightmapCache;
    static int sCompositeCacheWidth = 0;
    static int sCompositeCacheHeight = 0;
    static uint64_t sCompositeCacheGeneration = 0;
    static std::mutex sCompositeCacheMutex;

    // Composite heightmap generation progress
    static std::atomic<bool> sCompositeGenerating{false};
    static std::atomic<int> sCompositeProgressCurrent{0};
    static std::atomic<int> sCompositeProgressTotal{0};

    // Heightmap viewer loading state
    static bool sHeightmapWaitingForGeneration = false;
    static std::string sHeightmapWaitingFolderName;

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

    bool IsCompositeHeightmapReady()
    {
        std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
        return !sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == sCurrentGeneration;
    }

    // Heightmap export state
    static std::vector<uint8_t> sHeightmapPixels;
    static std::string sHeightmapExportName;

    static std::shared_ptr<FileEntry> FindFileRecursive(const IFileSystemEntryPtr& entry, const std::string& targetLower)
    {
        if (!entry) return nullptr;
        std::string name = wstring_to_utf8(entry->getEntryName());

        if (!entry->isDirectory()) {
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == targetLower)
                return std::dynamic_pointer_cast<FileEntry>(entry);
            return nullptr;
        }

        for (const auto& child : entry->getChildren()) {
            auto result = FindFileRecursive(child, targetLower);
            if (result) return result;
        }
        return nullptr;
    }

    static void FindAllBnkFiles(const IFileSystemEntryPtr& entry, std::vector<std::pair<std::shared_ptr<FileEntry>, std::string>>& results)
    {
        if (!entry) return;
        std::string name = wstring_to_utf8(entry->getEntryName());

        if (!entry->isDirectory()) {
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.size() > 4 && lower.substr(lower.size() - 4) == ".bnk")
                results.push_back({std::dynamic_pointer_cast<FileEntry>(entry), lower});
            return;
        }

        for (const auto& child : entry->getChildren())
            FindAllBnkFiles(child, results);
    }

    enum class AudioInitPhase {
        NotStarted,
        LoadingSoundEventTable,
        LoadingEventsBnk,
        ScanningBanks,
        ProcessingBanks,
        Finalizing,
        Complete
    };

    static AudioInitPhase sAudioInitPhase = AudioInitPhase::NotStarted;
    static std::vector<std::pair<ArchivePtr, std::pair<std::shared_ptr<FileEntry>, std::string>>> sPendingBnkFiles;
    static size_t sBnkProcessIndex = 0;

    void InitializeAudioDatabase(AppState& state)
    {
        if (sWemResolver.isLoaded()) {
            state.audioInitComplete = true;
            sAudioInitPhase = AudioInitPhase::Complete;
            return;
        }

        switch (sAudioInitPhase) {
        case AudioInitPhase::NotStarted:
            sWemResolver.clear();
            sPendingBnkFiles.clear();
            sBnkProcessIndex = 0;
            sAudioInitPhase = AudioInitPhase::LoadingSoundEventTable;
            state.audioInitStatus = "Loading SoundEvent.tbl...";
            break;

        case AudioInitPhase::LoadingSoundEventTable:
            for (const auto& archive : state.archives) {
                if (!archive) continue;
                auto tblFile = FindFileRecursive(archive->getRoot(), "soundevent.tbl");
                if (tblFile) {
                    std::vector<uint8_t> tblData;
                    if (archive->getFileData(tblFile, tblData) && !tblData.empty()) {
                        sWemResolver.loadSoundEventTable(tblData.data(), tblData.size());
                    }
                    break;
                }
            }
            sAudioInitPhase = AudioInitPhase::LoadingEventsBnk;
            state.audioInitStatus = "Loading Events.bnk...";
            break;

        case AudioInitPhase::LoadingEventsBnk:
            for (const auto& archive : state.archives) {
                if (!archive) continue;
                auto bnkFile = FindFileRecursive(archive->getRoot(), "events.bnk");
                if (bnkFile) {
                    std::vector<uint8_t> bnkData;
                    if (archive->getFileData(bnkFile, bnkData) && !bnkData.empty()) {
                        sWemResolver.loadEventsBnk(bnkData.data(), bnkData.size());
                    }
                    break;
                }
            }
            sAudioInitPhase = AudioInitPhase::ScanningBanks;
            state.audioInitStatus = "Scanning audio banks...";
            break;

        case AudioInitPhase::ScanningBanks:
            sPendingBnkFiles.clear();
            for (const auto& archive : state.archives) {
                if (!archive) continue;
                std::vector<std::pair<std::shared_ptr<FileEntry>, std::string>> bnkFiles;
                FindAllBnkFiles(archive->getRoot(), bnkFiles);
                for (auto& bf : bnkFiles) {
                    if (bf.second != "events.bnk" && bf.second != "init.bnk") {
                        sPendingBnkFiles.push_back({archive, std::move(bf)});
                    }
                }
            }
            sBnkProcessIndex = 0;
            sAudioInitPhase = AudioInitPhase::ProcessingBanks;
            break;

        case AudioInitPhase::ProcessingBanks:
        {
            const int banksPerFrame = 5;
            int processed = 0;
            while (sBnkProcessIndex < sPendingBnkFiles.size() && processed < banksPerFrame) {
                const auto& [archive, bnkInfo] = sPendingBnkFiles[sBnkProcessIndex];
                const auto& [bnkFile, lowerName] = bnkInfo;

                state.audioInitStatus = "Processing " + lowerName + " (" +
                    std::to_string(sBnkProcessIndex + 1) + "/" +
                    std::to_string(sPendingBnkFiles.size()) + ")";

                std::vector<uint8_t> bnkData;
                if (archive->getFileData(bnkFile, bnkData) && !bnkData.empty()) {
                    sWemResolver.loadAudioBnk(bnkData.data(), bnkData.size());
                }

                sBnkProcessIndex++;
                processed++;
            }

            if (sBnkProcessIndex >= sPendingBnkFiles.size()) {
                sAudioInitPhase = AudioInitPhase::Finalizing;
                state.audioInitStatus = "Finalizing audio database...";
            }
            break;
        }

        case AudioInitPhase::Finalizing:
            sWemResolver.finalize();
            sPendingBnkFiles.clear();
            sAudioInitPhase = AudioInitPhase::Complete;
            state.audioInitComplete = true;
            break;

        case AudioInitPhase::Complete:
            state.audioInitComplete = true;
            break;
        }
    }

    void LoadWemNameLookup(const std::vector<ArchivePtr>& archives)
    {
        if (sWemResolver.isLoaded()) {
            return;
        }

        sWemResolver.clear();

        for (const auto& archive : archives) {
            if (!archive) continue;
            auto tblFile = FindFileRecursive(archive->getRoot(), "soundevent.tbl");
            if (tblFile) {
                std::vector<uint8_t> tblData;
                if (archive->getFileData(tblFile, tblData) && !tblData.empty()) {
                    sWemResolver.loadSoundEventTable(tblData.data(), tblData.size());
                }
                break;
            }
        }

        for (const auto& archive : archives) {
            if (!archive) continue;
            auto bnkFile = FindFileRecursive(archive->getRoot(), "events.bnk");
            if (bnkFile) {
                std::vector<uint8_t> bnkData;
                if (archive->getFileData(bnkFile, bnkData) && !bnkData.empty()) {
                    sWemResolver.loadEventsBnk(bnkData.data(), bnkData.size());
                }
                break;
            }
        }

        for (const auto& archive : archives) {
            if (!archive) continue;
            std::vector<std::pair<std::shared_ptr<FileEntry>, std::string>> bnkFiles;
            FindAllBnkFiles(archive->getRoot(), bnkFiles);

            for (const auto& [bnkFile, lowerName] : bnkFiles) {
                if (lowerName == "events.bnk" || lowerName == "init.bnk") continue;

                std::vector<uint8_t> bnkData;
                if (archive->getFileData(bnkFile, bnkData) && !bnkData.empty()) {
                    sWemResolver.loadAudioBnk(bnkData.data(), bnkData.size());
                }
            }
        }

        sWemResolver.finalize();
    }

    std::string GetWemDisplayName(uint32_t id)
    {
        return sWemResolver.resolve(id);
    }

    bool BuildPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target, std::vector<IFileSystemEntryPtr>& outPath)
    {
        if (current.get() == target.get())
        {
            outPath.push_back(current);
            return true;
        }

        if (!current->isDirectory()) return false;

        for (const auto& child : current->getChildren())
        {
            if (child && child->isDirectory())
            {
                if (BuildPathToNode(child, target, outPath))
                {
                    outPath.insert(outPath.begin(), current);
                    return true;
                }
            }
        }
        return false;
    }

    static bool GenerateCompositeThumbnailFromFiles(
        const std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>>& areaFiles,
        std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight,
        uint64_t generation)
    {
        if (areaFiles.empty()) return false;

        sCompositeGenerating = true;
        sCompositeProgressTotal = static_cast<int>(areaFiles.size());
        sCompositeProgressCurrent = 0;

        // Determine thread count - use hardware threads but cap at reasonable number
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
        numThreads = std::min(numThreads, 8u);
        numThreads = std::min(numThreads, static_cast<unsigned int>(areaFiles.size()));

        // Parse area files in parallel batches
        std::vector<ParsedArea> parsedAreas;
        parsedAreas.reserve(areaFiles.size());
        std::mutex resultsMutex;

        size_t totalFiles = areaFiles.size();
        size_t processed = 0;

        while (processed < totalFiles)
        {
            // Launch batch of async tasks
            std::vector<std::future<ParsedArea>> futures;
            size_t batchEnd = std::min(processed + numThreads, totalFiles);

            for (size_t i = processed; i < batchEnd; i++)
            {
                const auto& [arc, fe] = areaFiles[i];
                futures.push_back(std::async(std::launch::async, [arc, fe]() {
                    return AreaFile::parseAreaFile(arc, fe);
                }));
            }

            // Collect results from this batch
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

        // Calculate bounds from all parsed areas
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

        // Autocrop
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

        // Generate full-resolution RGBA and cache it
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

        // Store in cache
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            sCompositeHeightmapCache = fullResPixels;
            sCompositeCacheWidth = croppedWidth;
            sCompositeCacheHeight = croppedHeight;
            sCompositeCacheGeneration = generation;
        }

        sCompositeGenerating = false;

        // Create thumbnail - downscale to max 256x256 maintaining aspect ratio
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

    void ThumbnailWorker()
    {
        while (sWorkerRunning)
        {
            ThumbnailRequest req;
            bool hasJob = false;

            {
                std::unique_lock<std::mutex> lock(sQueueMutex);
                sQueueCV.wait(lock, [] { return !sLoadQueue.empty() || !sWorkerRunning; });

                if (!sWorkerRunning) break;

                if (!sLoadQueue.empty())
                {
                    req = sLoadQueue.front();
                    sLoadQueue.pop_front();
                    hasJob = true;
                }
            }

            if (hasJob)
            {
                ThumbnailResult res;
                res.fileIndex = req.fileIndex;
                res.generation = req.generation;
                res.success = false;

                // Handle composite thumbnail request
                if (req.isComposite)
                {
                    std::vector<uint8_t> pixels;
                    int w, h;
                    if (GenerateCompositeThumbnailFromFiles(req.compositeAreaFiles, pixels, w, h, req.generation))
                    {
                        res.width = w;
                        res.height = h;
                        res.data = std::move(pixels);
                        res.success = true;
                    }
                }
                else if (req.archive && req.entry)
                {
                    if (req.extension == ".area")
                    {
                        auto parsed = AreaFile::parseAreaFile(req.archive, req.entry);
                        if (parsed.valid)
                        {
                            int w = 256;
                            int h = 256;
                            std::vector<uint8_t> pixels(w * h * 4, 0);

                            float minH = parsed.minBounds.y;
                            float maxH = parsed.maxHeight;
                            float range = maxH - minH;
                            if (range < 1.0f) range = 1.0f;

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
                                            float norm = (height - minH) / range;
                                            uint8_t val = (uint8_t)(std::clamp(norm, 0.0f, 1.0f) * 255.0f);

                                            int px = cx * 16 + lx;
                                            int py = cz * 16 + lz;
                                            int pIdx = (py * w + px) * 4;

                                            pixels[pIdx + 0] = val;
                                            pixels[pIdx + 1] = val;
                                            pixels[pIdx + 2] = val;
                                            pixels[pIdx + 3] = 255;
                                        }
                                    }
                                }
                            }
                            res.width = w;
                            res.height = h;
                            res.data = std::move(pixels);
                            res.success = true;
                        }
                    }
                    else
                    {
                        std::vector<uint8_t> bytes;
                        if (req.archive->getFileData(req.entry, bytes) && !bytes.empty())
                        {
                            Tex::File tf;
                            if (tf.readFromMemory(bytes.data(), bytes.size()))
                            {
                                Tex::ImageRGBA img;
                                if (tf.decodeLargestMipToRGBA(img))
                                {
                                    res.width = img.width;
                                    res.height = img.height;
                                    res.data = std::move(img.rgba);
                                    res.success = true;
                                }
                            }
                        }
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(sQueueMutex);
                    sResultQueue.push_back(std::move(res));
                }
            }
        }
    }

    void EnsureWorkerStarted()
    {
        if (!sWorkerRunning)
        {
            sWorkerRunning = true;
            sWorkerThread = std::thread(ThumbnailWorker);
            sWorkerThread.detach();
        }
    }

    static ID3D11ShaderResourceView* CreateTextureFromRGBA(const uint8_t* data, int width, int height)
    {
        if (!gDevice || !data || width <= 0 || height <= 0) return nullptr;

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
        if (FAILED(hr)) return nullptr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = gDevice->CreateShaderResourceView(texture, &srvDesc, &srv);
        texture->Release();

        if (FAILED(hr)) return nullptr;
        return srv;
    }

    void ProcessThumbnailResults()
    {
        std::lock_guard<std::mutex> lock(sQueueMutex);
        while (!sResultQueue.empty())
        {
            auto res = sResultQueue.front();
            sResultQueue.pop_front();

            if (res.generation != sCurrentGeneration) continue;
            if (res.fileIndex < 0 || res.fileIndex >= (int)sCachedFiles.size()) continue;

            auto& file = sCachedFiles[res.fileIndex];
            if (res.success && !file.textureID)
            {
                ID3D11ShaderResourceView* srv = CreateTextureFromRGBA(res.data.data(), res.width, res.height);
                file.textureID = reinterpret_cast<ImTextureID>(srv);
                file.texWidth = res.width;
                file.texHeight = res.height;
            }
        }
    }

    std::string GetExtension(const std::string& filename)
    {
        size_t dot = filename.rfind('.');
        if (dot != std::string::npos)
            return ToLowerCopy(filename.substr(dot));
        return "";
    }

    bool FindPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target)
    {
        if (current.get() == target.get())
        {
            sNodesToExpand.insert(current.get());
            return true;
        }
        if (!current->isDirectory()) return false;

        for (const auto& child : current->getChildren())
        {
            if (child && child->isDirectory())
            {
                if (FindPathToNode(child, target))
                {
                    sNodesToExpand.insert(current.get());
                    return true;
                }
            }
        }
        return false;
    }

    static std::string GetWemDisplayNameFromFilename(const std::string& filename)
    {
        size_t dotPos = filename.rfind('.');
        std::string baseName = (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;

        try {
            uint32_t id = std::stoul(baseName);
            std::string resolved = GetWemDisplayName(id);
            if (resolved != baseName) {
                return resolved + ".wem";
            }
        } catch (...) {
        }
        return filename;
    }

    void CollectRecursive(const ArchivePtr& archive, const IFileSystemEntryPtr& folder, const std::string& filterLower, std::vector<FileInfo>& outList)
    {
        if (!folder || !folder->isDirectory()) return;

        for (const auto& child : folder->getChildren())
        {
            if (!child) continue;

            if (child->isDirectory())
            {
                CollectRecursive(archive, child, filterLower, outList);
            }
            else
            {
                std::string name = wstring_to_utf8(child->getEntryName());
                std::string ext = GetExtension(name);

                if (ext == ".wem") {
                    name = GetWemDisplayNameFromFilename(name);
                }

                auto searchIt = std::search(
                    name.begin(), name.end(),
                    filterLower.begin(), filterLower.end(),
                    [](char c1, char c2) {
                        return std::tolower(static_cast<unsigned char>(c1)) == c2;
                    }
                );

                if (searchIt != name.end())
                {
                    FileInfo info;
                    info.name = std::move(name);
                    info.extension = ext;
                    info.entry = child;
                    info.archive = archive;
                    info.isDirectory = false;
                    outList.push_back(std::move(info));
                }
            }
        }
    }

    void RefreshFileList(AppState& state)
    {
        {
            std::lock_guard<std::mutex> lock(sQueueMutex);
            sCurrentGeneration++;
            sLoadQueue.clear();
            sResultQueue.clear();
        }

        // Clear composite heightmap cache and generating state
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

        for (const auto& file : sCachedFiles)
        {
            if (file.textureID)
            {
                ID3D11ShaderResourceView* srv = reinterpret_cast<ID3D11ShaderResourceView*>(file.textureID);
                if (srv) srv->Release();
            }
        }
        sCachedFiles.clear();

        sBreadcrumbPath.clear();
        if (sSelectedFolder && sSelectedArchive)
        {
            auto root = sSelectedArchive->getRoot();
            if (root)
            {
                BuildPathToNode(root, sSelectedFolder, sBreadcrumbPath);
            }
        }

        LoadWemNameLookup(state.archives);

        bool isFiltering = (sSearchFilter[0] != '\0');
        std::string filterLower;
        if (isFiltering) {
            filterLower = sSearchFilter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        }

        if (sBnkViewActive && !sBnkWemEntries.empty())
        {
            for (const auto& wemEntry : sBnkWemEntries)
            {
                FileInfo info;
                info.name = wemEntry.displayName + ".wem";
                info.extension = ".wem";
                info.entry = nullptr;
                info.archive = sBnkViewArchive;
                info.isDirectory = false;

                if (isFiltering) {
                    std::string nameLower = info.name;
                    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                    if (nameLower.find(filterLower) == std::string::npos) continue;
                }

                sCachedFiles.push_back(info);
            }
        }
        else if (!sSelectedFolder)
        {
            for (const auto& archive : state.archives)
            {
                if (!archive) continue;
                auto root = archive->getRoot();
                if (!root) continue;

                if (isFiltering)
                {
                    CollectRecursive(archive, root, filterLower, sCachedFiles);
                }
                else
                {
                    FileInfo info;
                    std::filesystem::path p(archive->getPath());
                    info.name = p.filename().string();
                    info.extension = "";
                    info.entry = root;
                    info.archive = archive;
                    info.isDirectory = true;
                    sCachedFiles.push_back(info);
                }
            }
        }
        else if (sSelectedArchive)
        {
            if (isFiltering)
            {
                CollectRecursive(sSelectedArchive, sSelectedFolder, filterLower, sCachedFiles);
            }
            else
            {
                for (const auto& child : sSelectedFolder->getChildren())
                {
                    if (!child) continue;

                    FileInfo info;
                    info.name = wstring_to_utf8(child->getEntryName());
                    info.extension = GetExtension(info.name);
                    info.entry = child;
                    info.archive = sSelectedArchive;
                    info.isDirectory = child->isDirectory();

                    if (info.extension == ".wem") {
                        info.name = GetWemDisplayNameFromFilename(info.name);
                    }

                    sCachedFiles.push_back(info);
                }
            }
        }

        std::sort(sCachedFiles.begin(), sCachedFiles.end(), [](const FileInfo& a, const FileInfo& b) {
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });

        if (sSelectedFolder && !isFiltering)
        {
            bool hasAreaFiles = false;
            for (const auto& f : sCachedFiles)
            {
                if (!f.isDirectory && f.extension == ".area")
                {
                    hasAreaFiles = true;
                    break;
                }
            }

            if (hasAreaFiles)
            {
                std::string folderName = wstring_to_utf8(sSelectedFolder->getEntryName());

                FileInfo loadAllEntry;
                loadAllEntry.name = "Load " + folderName;
                loadAllEntry.extension = ".loadall";
                loadAllEntry.entry = nullptr;
                loadAllEntry.archive = sSelectedArchive;
                loadAllEntry.isDirectory = false;
                loadAllEntry.isLoadAllEntry = true;
                sCachedFiles.insert(sCachedFiles.begin(), loadAllEntry);
            }
        }

        sNeedsRefresh = false;
    }

    void LoadSingleArea(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
    {
        if (!arc || !fileEntry) return;

        ResetAreaReferencePosition();

        gLoadedAreas.clear();
        gSelectedChunk = nullptr;
        gSelectedChunkIndex = -1;
        gSelectedAreaIndex = -1;
        gSelectedAreaName.clear();
        gLoadedModel = nullptr;

        auto af = std::make_shared<AreaFile>(arc, fileEntry);
        if (af->load())
        {
            gLoadedAreas.push_back(af);
            state.currentArea = af;
            SnapCameraToLoaded(state);

            af->loadAllPropsAsync();
            gShowProps = true;
        }
        else
        {
            state.currentArea.reset();
        }
    }

    void LoadAllAreasInFolder(AppState& state)
    {
        if (!sSelectedFolder || !sSelectedArchive) return;

        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> areaFiles;
        for (const auto& file : sCachedFiles)
        {
            if (!file.isDirectory && file.extension == ".area")
            {
                auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                if (fe)
                    areaFiles.push_back({file.archive, fe});
            }
        }

        if (areaFiles.empty()) return;

        ResetAreaReferencePosition();
        Sky::Manager::Instance().clear();

        gLoadedAreas.clear();
        gSelectedChunk = nullptr;
        gSelectedChunkIndex = -1;
        gSelectedAreaIndex = -1;
        gSelectedAreaName.clear();
        gLoadedModel = nullptr;
        state.currentArea.reset();

        for (const auto& [arc, fileEntry] : areaFiles)
        {
            auto af = std::make_shared<AreaFile>(arc, fileEntry);
            if (af->load())
            {
                gLoadedAreas.push_back(af);
                if (!state.currentArea)
                    state.currentArea = af;
            }
        }

        Sky::Manager::Instance().clear();

        if (!gLoadedAreas.empty())
        {
            SnapCameraToLoaded(state);
            for (auto& area : gLoadedAreas)
                area->loadAllPropsAsync();
            gShowProps = true;
        }
    }

    void LoadSingleM3(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
    {
        if (!arc || !fileEntry) return;

        state.currentArea.reset();
        state.m3Render = nullptr;
        state.show_models_window = false;

        gPendingModelArchive = arc;

        std::string name = wstring_to_utf8(fileEntry->getEntryName());
        StartLoadingModel(arc, fileEntry, name);
    }

    void HandleFileOpen(AppState& state, const FileInfo& file)
    {
        if (file.isLoadAllEntry)
        {
            LoadAllAreasInFolder(state);
            return;
        }

        if (file.isDirectory)
        {
            sSelectedFolder = file.entry;
            sSelectedArchive = file.archive;
            sSearchFilter[0] = '\0';

            sRequestTreeSync = true;
            sNeedsRefresh = true;
            return;
        }

        auto fileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

        if (!fileEntry && !(sBnkViewActive && file.extension == ".wem")) return;

        if (file.extension == ".area")
        {
            LoadSingleArea(state, file.archive, fileEntry);
        }
        else if (file.extension == ".m3")
        {
            LoadSingleM3(state, file.archive, fileEntry);
        }
        else if (file.extension == ".tex")
        {
            if (file.textureID && file.texWidth > 0 && file.texHeight > 0)
            {
                Tex::OpenTexPreviewFromSRV(state,
                    reinterpret_cast<ID3D11ShaderResourceView*>(file.textureID),
                    file.texWidth, file.texHeight, "Texture Preview");
            }
            else
            {
                Tex::OpenTexPreviewFromEntry(state, file.archive, fileEntry);
            }
        }
        else if (file.extension == ".tbl")
        {
            UI_Tables::OpenTblFile(state, file.archive, fileEntry);
        }
        else if (file.extension == ".wem")
        {
            std::vector<uint8_t> wemData;

            if (fileEntry) {
                file.archive->getFileData(fileEntry, wemData);
            } else if (sBnkViewActive && !sBnkViewData.empty()) {
                std::string baseName = file.name;
                size_t dotPos = baseName.rfind('.');
                if (dotPos != std::string::npos)
                    baseName = baseName.substr(0, dotPos);

                const BnkWemEntry* foundEntry = nullptr;
                for (const auto& entry : sBnkWemEntries) {
                    if (entry.displayName == baseName) {
                        foundEntry = &entry;
                        break;
                    }
                }

                if (foundEntry) {
                    const uint8_t* data = sBnkViewData.data();
                    size_t size = sBnkViewData.size();
                    size_t pos = 0;
                    const uint8_t* dataSection = nullptr;

                    while (pos + 8 <= size) {
                        char sectionId[5] = {0};
                        memcpy(sectionId, data + pos, 4);
                        uint32_t sectionSize = *(uint32_t*)(data + pos + 4);

                        if (memcmp(sectionId, "DATA", 4) == 0) {
                            dataSection = data + pos + 8;
                            break;
                        }
                        pos += 8 + sectionSize;
                    }

                    if (dataSection) {
                        wemData.assign(dataSection + foundEntry->offset,
                                       dataSection + foundEntry->offset + foundEntry->size);
                    }
                }
            }

            if (!wemData.empty())
            {
                Audio::AudioPlayerWidget::Get().PlayFile(wemData.data(), wemData.size(), file.name);
            }
        }
        else if (file.extension == ".bnk")
        {
            std::vector<uint8_t> bnkData;
            if (file.archive->getFileData(fileEntry, bnkData) && !bnkData.empty())
            {
                LoadWemNameLookup(state.archives);

                sWemResolver.loadAudioBnk(bnkData.data(), bnkData.size());
                sWemResolver.finalize();

                sBnkWemEntries.clear();

                const uint8_t* data = bnkData.data();
                size_t size = bnkData.size();
                size_t pos = 0;

                const uint8_t* dataSection = nullptr;

                while (pos + 8 <= size) {
                    char sectionId[5] = {0};
                    memcpy(sectionId, data + pos, 4);
                    uint32_t sectionSize = *(uint32_t*)(data + pos + 4);

                    if (memcmp(sectionId, "DIDX", 4) == 0) {
                        const uint8_t* didx = data + pos + 8;
                        size_t numEntries = sectionSize / 12;
                        for (size_t i = 0; i < numEntries; i++) {
                            BnkWemEntry entry;
                            entry.id = *(uint32_t*)(didx + i * 12);
                            entry.offset = *(uint32_t*)(didx + i * 12 + 4);
                            entry.size = *(uint32_t*)(didx + i * 12 + 8);
                            entry.displayName = GetWemDisplayName(entry.id);
                            sBnkWemEntries.push_back(entry);
                        }
                    } else if (memcmp(sectionId, "DATA", 4) == 0) {
                        dataSection = data + pos + 8;
                    }

                    pos += 8 + sectionSize;
                }

                if (!sBnkWemEntries.empty() && dataSection) {
                    sBnkViewActive = true;
                    sBnkViewData = std::move(bnkData);
                    sBnkViewName = file.name;
                    sBnkViewArchive = file.archive;
                    sNeedsRefresh = true;
                }
            }
        }
    }

    IFileSystemEntryPtr FindFolderByPath(const IFileSystemEntryPtr& root, const std::vector<std::string>& pathParts, size_t startIndex)
    {
        if (!root || !root->isDirectory()) return nullptr;
        if (startIndex >= pathParts.size()) return root;

        const std::string& targetName = pathParts[startIndex];

        for (const auto& child : root->getChildren())
        {
            if (!child || !child->isDirectory()) continue;

            std::string childName = wstring_to_utf8(child->getEntryName());

            bool match = (childName.size() == targetName.size());
            if (match)
            {
                for (size_t i = 0; i < childName.size() && match; ++i)
                {
                    if (std::tolower(static_cast<unsigned char>(childName[i])) !=
                        std::tolower(static_cast<unsigned char>(targetName[i])))
                        match = false;
                }
            }

            if (match)
            {
                if (startIndex + 1 >= pathParts.size())
                    return child;
                return FindFolderByPath(child, pathParts, startIndex + 1);
            }
        }
        return nullptr;
    }

    std::vector<std::string> SplitPath(const std::string& path)
    {
        std::vector<std::string> parts;
        std::string current;

        for (char c : path)
        {
            if (c == '/' || c == '\\')
            {
                if (!current.empty())
                {
                    parts.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }
        if (!current.empty())
            parts.push_back(current);

        return parts;
    }

    std::string ExtractFolderPath(const std::string& filePath)
    {
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            return filePath.substr(0, lastSlash);
        return "";
    }

    // Heightmap viewer state
    static bool sHeightmapViewerOpen = false;
    static std::string sHeightmapTitle;
    static ID3D11ShaderResourceView* sHeightmapSRV = nullptr;
    static int sHeightmapWidth = 0;
    static int sHeightmapHeight = 0;

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

    void GenerateAllAreasHeightmap(const std::string& folderName)
    {
        // Check if we have cached composite heightmap from background generation
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            if (!sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == sCurrentGeneration)
            {
                // Use cached data
                sHeightmapPixels = sCompositeHeightmapCache;
                sHeightmapWidth = sCompositeCacheWidth;
                sHeightmapHeight = sCompositeCacheHeight;
                sHeightmapExportName = folderName;
                return;
            }
        }

        // No cache, generate from scratch
        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> areaFiles;
        for (const auto& file : sCachedFiles)
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

        // Autocrop
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

    void ViewAllAreasHeightmap(AppState& state, const std::string& folderName)
    {
        // Count area files for the title
        int areaCount = 0;
        for (const auto& file : sCachedFiles)
        {
            if (!file.isDirectory && file.extension == ".area" && !file.isLoadAllEntry)
                areaCount++;
        }

        // Check if we have cached composite heightmap from background generation
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            if (!sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == sCurrentGeneration)
            {
                // Use cached data - create texture from it
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

        // If still generating, open viewer with loading state
        if (sCompositeGenerating)
        {
            sHeightmapTitle = "Heightmap: " + folderName + " (" + std::to_string(areaCount) + " areas)";
            sHeightmapWaitingFolderName = folderName;
            sHeightmapWaitingForGeneration = true;
            sHeightmapViewerOpen = true;
            return;
        }

        // No cache and not generating - shouldn't happen normally, but generate from scratch
        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> areaFiles;
        for (const auto& file : sCachedFiles)
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

        // Autocrop
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

    void DrawHeightmapViewer()
    {
        // Always handle export dialogs, even if viewer is closed
        if (ImGuiFileDialog::Instance()->Display("ExportHeightmapPngDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                if (sPendingHeightmapType == PendingHeightmapType::Single)
                    GenerateSingleAreaHeightmap(sPendingHeightmapArchive, sPendingHeightmapEntry, sPendingHeightmapName);
                else if (sPendingHeightmapType == PendingHeightmapType::All)
                    GenerateAllAreasHeightmap(sPendingHeightmapName);
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
                    GenerateAllAreasHeightmap(sPendingHeightmapName);
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
                    GenerateAllAreasHeightmap(sPendingHeightmapName);
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
                    GenerateAllAreasHeightmap(sPendingHeightmapName);
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

        // If waiting for generation, check if it's done
        if (sHeightmapWaitingForGeneration)
        {
            std::lock_guard<std::mutex> lock(sCompositeCacheMutex);
            if (!sCompositeHeightmapCache.empty() && sCompositeCacheGeneration == sCurrentGeneration)
            {
                // Generation complete - load the data
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
            // Show loading progress if waiting
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

                // Right-click context menu on the image
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