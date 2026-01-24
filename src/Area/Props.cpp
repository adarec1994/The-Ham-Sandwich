#define NOMINMAX
#include "Props.h"
#include "../Archive.h"
#include "../models/M3Loader.h"
#include "../models/M3LoaderV95.h"
#include "../models/M3Render.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <unordered_set>

// ============================================================================
// TEXTURE FAILURE TRACKING - Only logs M3 files that fail to load textures
// ============================================================================
static std::atomic<int> g_TexturesFailed{0};

// Track models with texture loading issues
static std::mutex g_TextureFailureMutex;
static std::unordered_map<std::string, std::vector<std::string>> g_FailedTexturePaths;
static std::unordered_set<std::string> g_ModelsWithTextureIssues;

void RecordTextureFailure(const std::string& modelPath, const std::string& texturePath)
{
    std::lock_guard<std::mutex> lock(g_TextureFailureMutex);

    // Only print once per model+texture combination
    auto& texList = g_FailedTexturePaths[modelPath];
    for (const auto& existing : texList)
    {
        if (existing == texturePath) return; // Already recorded
    }

    texList.push_back(texturePath);
    g_ModelsWithTextureIssues.insert(modelPath);
    g_TexturesFailed++;

    // Print immediately for visibility
    std::cerr << "[M3 TEXTURE FAIL] " << modelPath << " -> " << texturePath << std::endl;
}

// Debug: print prop fields to find variant selector (disabled - unk7 is variant)
void DebugPrintPropFields(const Prop&) {}

void PrintFailedTextures()
{
    std::lock_guard<std::mutex> lock(g_TextureFailureMutex);
    if (g_FailedTexturePaths.empty()) return;

    std::cout << "\n========== M3 TEXTURE FAILURES ==========\n";
    for (const auto& [model, textures] : g_FailedTexturePaths)
    {
        std::cout << model << "\n";
        for (const auto& tex : textures)
            std::cout << "  -> " << tex << "\n";
    }
    std::cout << "Models: " << g_ModelsWithTextureIssues.size()
              << " | Textures: " << g_TexturesFailed.load() << "\n";
    std::cout << "==========================================\n" << std::endl;
}

// Stub functions - do nothing
static void PropDebugLog(const std::string&) {}
static void PropDebugLogWarning(const std::string&) {}
static void PropDebugLogError(const std::string&) {}

void PrintPropTextureDebugSummary()
{
    PrintFailedTextures();
}

void ResetPropTextureDebugCounters()
{
    g_TexturesFailed = 0;
    std::lock_guard<std::mutex> lock(g_TextureFailureMutex);
    g_FailedTexturePaths.clear();
    g_ModelsWithTextureIssues.clear();
}

void SetPropTextureDebugEnabled(bool) {}

constexpr size_t PROP_ENTRY_SIZE = 104;

std::string ReadWideString(const uint8_t* data, size_t offset, size_t maxLen)
{
    std::string result;
    result.reserve(256);

    const uint16_t* wptr = reinterpret_cast<const uint16_t*>(data + offset);
    for (size_t i = 0; i < maxLen; i++)
    {
        uint16_t wc = wptr[i];
        if (wc == 0) break;

        if (wc < 128)
            result += static_cast<char>(wc);
        else
            result += '?';
    }

    return result;
}

void ParsePropsChunk(const uint8_t* data, size_t size, std::vector<Prop>& outProps, std::unordered_map<uint32_t, size_t>& outLookup)
{
    if (!data || size < 4) return;

    uint32_t propCount = static_cast<uint32_t>(data[0]) |
                        (static_cast<uint32_t>(data[1]) << 8) |
                        (static_cast<uint32_t>(data[2]) << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);

    outProps.clear();
    outLookup.clear();
    outProps.reserve(propCount);

    const uint8_t* ptr = data + 4;

    for (uint32_t i = 0; i < propCount; i++)
    {
        if (4 + i * PROP_ENTRY_SIZE + PROP_ENTRY_SIZE > size)
            break;

        const uint8_t* propPtr = ptr + i * PROP_ENTRY_SIZE;
        Prop prop;

        size_t offset = 0;

        std::memcpy(&prop.uniqueID, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.someID, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk0, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk1, propPtr + offset, 4); offset += 4;

        int32_t modelType;
        std::memcpy(&modelType, propPtr + offset, 4); offset += 4;
        prop.modelType = static_cast<PropModelType>(modelType);

        std::memcpy(&prop.nameOffset, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unkOffset, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.scale, propPtr + offset, 4); offset += 4;

        float qx, qy, qz, qw;
        std::memcpy(&qx, propPtr + offset, 4); offset += 4;
        std::memcpy(&qy, propPtr + offset, 4); offset += 4;
        std::memcpy(&qz, propPtr + offset, 4); offset += 4;
        std::memcpy(&qw, propPtr + offset, 4); offset += 4;
        prop.rotation = glm::quat(qw, qx, qy, qz);

        float px, py, pz;
        std::memcpy(&px, propPtr + offset, 4); offset += 4;
        std::memcpy(&py, propPtr + offset, 4); offset += 4;
        std::memcpy(&pz, propPtr + offset, 4); offset += 4;
        prop.position = glm::vec3(px, py, pz);

        std::memcpy(&prop.placement.minX, propPtr + offset, 2); offset += 2;
        std::memcpy(&prop.placement.minY, propPtr + offset, 2); offset += 2;
        std::memcpy(&prop.placement.maxX, propPtr + offset, 2); offset += 2;
        std::memcpy(&prop.placement.maxY, propPtr + offset, 2); offset += 2;

        std::memcpy(&prop.unk7, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk8, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk9, propPtr + offset, 4); offset += 4;

        std::memcpy(&prop.color0, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.color1, propPtr + offset, 4); offset += 4;

        std::memcpy(&prop.unk10, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk11, propPtr + offset, 4); offset += 4;

        std::memcpy(&prop.color2, propPtr + offset, 4); offset += 4;
        std::memcpy(&prop.unk12, propPtr + offset, 4); offset += 4;

        if (prop.nameOffset != 0)
        {
            size_t stringOffset = static_cast<size_t>(prop.nameOffset);
            if (stringOffset < size)
            {
                prop.path = ReadWideString(data, stringOffset, 512);
            }
        }

        outLookup[prop.uniqueID] = outProps.size();
        outProps.push_back(std::move(prop));
    }
}

void ParseCurtsChunk(const uint8_t* data, size_t size, std::vector<CurtData>& outCurts)
{
    if (!data || size < 4) return;

    uint32_t curtCount = static_cast<uint32_t>(data[0]) |
                        (static_cast<uint32_t>(data[1]) << 8) |
                        (static_cast<uint32_t>(data[2]) << 16) |
                        (static_cast<uint32_t>(data[3]) << 24);

    outCurts.clear();
    outCurts.reserve(curtCount);

    const uint8_t* ptr = data + 4;
    const uint8_t* end = data + size;

    for (uint32_t i = 0; i < curtCount && ptr + 24 <= end; i++)
    {
        CurtData curt;

        std::memcpy(&curt.unk0, ptr, 4); ptr += 4;
        std::memcpy(&curt.positionCount, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.minX, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.minY, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.maxX, ptr, 2); ptr += 2;
        std::memcpy(&curt.placement.maxY, ptr, 2); ptr += 2;
        std::memcpy(&curt.unk5, ptr, 2); ptr += 2;
        std::memcpy(&curt.positionOffset, ptr, 4); ptr += 4;
        std::memcpy(&curt.unk6, ptr, 4); ptr += 4;

        if (curt.positionOffset != 0 && curt.positionCount > 0)
        {
            size_t posOffset = static_cast<size_t>(curt.positionOffset);
            if (posOffset + curt.positionCount * 12 <= size)
            {
                curt.positions.resize(curt.positionCount);
                const uint8_t* posPtr = data + posOffset;
                for (int j = 0; j < curt.positionCount; j++)
                {
                    float x, y, z;
                    std::memcpy(&x, posPtr, 4); posPtr += 4;
                    std::memcpy(&y, posPtr, 4); posPtr += 4;
                    std::memcpy(&z, posPtr, 4); posPtr += 4;
                    curt.positions[j] = glm::vec3(x, y, z);
                }
            }
        }

        outCurts.push_back(std::move(curt));
    }
}

static std::string NormalizePath(const std::string& path)
{
    std::string result = path;
    for (char& c : result)
    {
        if (c == '\\') c = '/';
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
    while (!result.empty() && result[0] == '/')
        result.erase(0, 1);
    return result;
}

PropLoader& PropLoader::Instance()
{
    static PropLoader instance;
    return instance;
}

PropLoader::~PropLoader()
{
    Shutdown();
}

void PropLoader::Initialize(size_t numThreads)
{
    if (mRunning.load()) return;

    if (numThreads == 0)
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);

    mRunning.store(true);
    mWorkers.reserve(numThreads);

    for (size_t i = 0; i < numThreads; ++i)
        mWorkers.emplace_back(&PropLoader::WorkerThread, this);
}

void PropLoader::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mWorkMutex);
        mRunning.store(false);
    }
    mWorkCondition.notify_all();

    for (auto& worker : mWorkers)
    {
        if (worker.joinable())
            worker.join();
    }
    mWorkers.clear();

    std::lock_guard<std::mutex> uploadLock(mUploadMutex);
    while (!mPendingUploads.empty())
    {
        auto& upload = mPendingUploads.front();
        delete upload.modelData;
        mPendingUploads.pop();
    }
}

void PropLoader::SetArchive(const ArchivePtr& archive)
{
    std::lock_guard<std::mutex> archiveLock(mArchiveMutex);
    std::lock_guard<std::mutex> fileLock(mFileCacheMutex);
    mArchive = archive;
    mFileCache.clear();
}

void PropLoader::ClearCache()
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    mModelCache.clear();

    std::lock_guard<std::mutex> fileLock(mFileCacheMutex);
    mFileCache.clear();
}

std::shared_ptr<M3Render> PropLoader::GetCachedModel(const std::string& path)
{
    std::string normalized = NormalizePath(path);
    std::lock_guard<std::mutex> lock(mCacheMutex);
    auto it = mModelCache.find(normalized);
    if (it != mModelCache.end() && it->second.valid)
        return it->second.render;
    return nullptr;
}

void PropLoader::CacheModel(const std::string& path, std::shared_ptr<M3Render> render)
{
    std::string normalized = NormalizePath(path);
    std::lock_guard<std::mutex> lock(mCacheMutex);
    CachedModel cached;
    cached.render = render;
    cached.valid = true;
    mModelCache[normalized] = cached;
}

size_t PropLoader::GetCacheSize() const
{
    std::lock_guard<std::mutex> lock(mCacheMutex);
    return mModelCache.size();
}

FileEntryPtr PropLoader::FindPropFile(const std::string& path)
{
    ArchivePtr archive = getArchive();
    if (!archive) return nullptr;

    std::string normalized = NormalizePath(path);

    {
        std::lock_guard<std::mutex> lock(mFileCacheMutex);
        auto it = mFileCache.find(normalized);
        if (it != mFileCache.end())
            return it->second;
    }

    FileEntryPtr fileEntry;

    {
        std::lock_guard<std::mutex> archiveLock(mArchiveAccessMutex);
        fileEntry = archive->findFileCached(normalized);

        if (!fileEntry)
        {
            std::string withExt = normalized;
            if (withExt.size() < 3 || withExt.substr(withExt.size() - 3) != ".m3")
            {
                withExt += ".m3";
                fileEntry = archive->findFileCached(withExt);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mFileCacheMutex);
        mFileCache[normalized] = fileEntry;
    }

    return fileEntry;
}

void PropLoader::QueueProp(Prop* prop)
{
    if (!mRunning.load())
        Initialize();

    if (!prop) return;

    bool expected = false;
    if (!prop->loadRequested.compare_exchange_strong(expected, true))
        return;

    if (prop->loaded.load(std::memory_order_acquire))
    {
        prop->loadRequested.store(false);
        return;
    }

    if (prop->modelType == PropModelType::Unk_2 || prop->modelType == PropModelType::Unk_4)
    {
        PropDebugLog("Skipping prop ID " + std::to_string(prop->uniqueID) + " - unsupported model type " + std::to_string(static_cast<int>(prop->modelType)));
        prop->loaded.store(true, std::memory_order_release);
        return;
    }

    if (prop->path.empty())
    {
        PropDebugLogWarning("Prop ID " + std::to_string(prop->uniqueID) + " has empty path");
        prop->loaded.store(true, std::memory_order_release);
        return;
    }

    std::string normalized = NormalizePath(prop->path);
    auto cached = GetCachedModel(normalized);
    if (cached)
    {
        PropDebugLog("Prop ID " + std::to_string(prop->uniqueID) + " using cached model: " + normalized);
        prop->render = cached;
        std::atomic_thread_fence(std::memory_order_release);
        prop->loaded.store(true, std::memory_order_release);
        return;
    }

    PropDebugLog("Queuing prop ID " + std::to_string(prop->uniqueID) + " path: " + normalized);
    mPendingCount.fetch_add(1, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(mWorkMutex);
        mWorkQueue.push(prop);
    }
    mWorkCondition.notify_one();
}

void PropLoader::QueueProps(std::vector<Prop*>& props)
{
    if (!mRunning.load())
    {
        Initialize();
    }

    std::vector<Prop*> toQueue;
    toQueue.reserve(props.size());

    int skippedUnsupportedType = 0;
    int skippedEmptyPath = 0;
    int usedCache = 0;

    for (Prop* prop : props)
    {
        if (!prop) continue;

        bool expected = false;
        if (!prop->loadRequested.compare_exchange_strong(expected, true))
            continue;

        if (prop->loaded.load(std::memory_order_acquire))
        {
            prop->loadRequested.store(false);
            continue;
        }

        if (prop->modelType == PropModelType::Unk_2 || prop->modelType == PropModelType::Unk_4)
        {
            skippedUnsupportedType++;
            prop->loaded.store(true, std::memory_order_release);
            continue;
        }

        if (prop->path.empty())
        {
            skippedEmptyPath++;
            prop->loaded.store(true, std::memory_order_release);
            continue;
        }

        std::string normalized = NormalizePath(prop->path);
        auto cached = GetCachedModel(normalized);
        if (cached)
        {
            usedCache++;
            prop->render = cached;
            std::atomic_thread_fence(std::memory_order_release);
            prop->loaded.store(true, std::memory_order_release);
            continue;
        }

        toQueue.push_back(prop);
    }

    PropDebugLog("QueueProps: " + std::to_string(props.size()) + " total, " +
                 std::to_string(toQueue.size()) + " queued, " +
                 std::to_string(usedCache) + " from cache, " +
                 std::to_string(skippedUnsupportedType) + " unsupported type, " +
                 std::to_string(skippedEmptyPath) + " empty path");

    if (toQueue.empty()) return;

    mPendingCount.fetch_add(toQueue.size(), std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(mWorkMutex);
        for (Prop* prop : toQueue)
            mWorkQueue.push(prop);
    }
    mWorkCondition.notify_all();
}

PropLoadResult PropLoader::LoadPropData(Prop* prop)
{
    PropLoadResult result;
    result.uniqueID = prop->uniqueID;
    result.path = prop->path;

    std::string normalized = NormalizePath(prop->path);

    {
        std::lock_guard<std::mutex> lock(mCacheMutex);
        auto it = mModelCache.find(normalized);
        if (it != mModelCache.end())
        {
            result.fromCache = true;
            result.success = it->second.valid;
            return result;
        }
    }

    FileEntryPtr fileEntry = FindPropFile(prop->path);
    if (!fileEntry)
    {
        PropDebugLogError("LoadPropData: File not found for path: " + prop->path);
        result.success = false;
        return result;
    }

    ArchivePtr archive = getArchive();
    if (!archive)
    {
        PropDebugLogError("LoadPropData: Archive is null for path: " + prop->path);
        result.success = false;
        return result;
    }

    std::vector<uint8_t> buffer;

    {
        std::lock_guard<std::mutex> archiveLock(mArchiveAccessMutex);
        if (!archive->getFileData(fileEntry, buffer) || buffer.empty())
        {
            PropDebugLogError("LoadPropData: Failed to read file data for: " + prop->path);
            result.success = false;
            return result;
        }
    }

    PropDebugLog("LoadPropData: Loaded " + std::to_string(buffer.size()) + " bytes for: " + normalized);

    M3ModelData* modelData = new M3ModelData();

    try
    {
        if (buffer.size() >= 8)
        {
            uint32_t version = 0;
            std::memcpy(&version, buffer.data() + 4, sizeof(version));
            PropDebugLog("  M3 version: " + std::to_string(version));
            if (version >= 90 && version < 100)
                *modelData = M3LoaderV95::Load(buffer);
            else
                *modelData = M3Loader::Load(buffer);
        }
        else
        {
            *modelData = M3Loader::Load(buffer);
        }

        if (!modelData->success)
        {
            PropDebugLogError("LoadPropData: M3Loader failed for: " + prop->path);
            delete modelData;
            result.success = false;
            return result;
        }

        PropDebugLog("  Model loaded successfully");
    }
    catch (const std::exception& e)
    {
        PropDebugLogError("LoadPropData: Exception loading " + prop->path + ": " + e.what());
        delete modelData;
        result.success = false;
        return result;
    }
    catch (...)
    {
        PropDebugLogError("LoadPropData: Unknown exception loading " + prop->path);
        delete modelData;
        result.success = false;
        return result;
    }

    result.modelData = modelData;
    result.success = true;
    return result;
}

void PropLoader::WorkerThread()
{
    while (true)
    {
        Prop* prop = nullptr;

        {
            std::unique_lock<std::mutex> lock(mWorkMutex);
            mWorkCondition.wait(lock, [this]
            {
                return !mWorkQueue.empty() || !mRunning.load();
            });

            if (!mRunning.load() && mWorkQueue.empty())
                return;

            if (mWorkQueue.empty())
                continue;

            prop = mWorkQueue.front();
            mWorkQueue.pop();
        }

        if (!prop)
        {
            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
            continue;
        }

        PropLoadResult result = LoadPropData(prop);

        if (result.fromCache)
        {
            std::shared_ptr<M3Render> cachedRender;
            {
                std::lock_guard<std::mutex> lock(mCacheMutex);
                std::string normalized = NormalizePath(prop->path);
                auto it = mModelCache.find(normalized);
                if (it != mModelCache.end() && it->second.valid)
                {
                    cachedRender = it->second.render;
                }
            }

            if (cachedRender)
            {
                prop->render = cachedRender;
                std::atomic_thread_fence(std::memory_order_release);
            }
            prop->loaded.store(true, std::memory_order_release);

            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
            continue;
        }

        if (result.success && result.modelData)
        {
            PendingUpload upload;
            upload.prop = prop;
            upload.modelData = result.modelData;
            upload.path = result.path;

            std::lock_guard<std::mutex> lock(mUploadMutex);
            mPendingUploads.push(upload);
        }
        else
        {
            prop->loaded.store(true, std::memory_order_release);
            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
        }
    }
}

void PropLoader::ProcessGPUUploads(int maxPerFrame)
{
    int processed = 0;

    ArchivePtr archive = getArchive();

    while (processed < maxPerFrame)
    {
        PendingUpload upload;

        {
            std::lock_guard<std::mutex> lock(mUploadMutex);
            if (mPendingUploads.empty())
                break;

            upload = mPendingUploads.front();
            mPendingUploads.pop();
        }

        if (!upload.prop || !upload.modelData)
        {
            PropDebugLogWarning("Skipping upload - null prop or modelData for path: " + upload.path);
            delete upload.modelData;
            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
            continue;
        }

        std::string normalized = NormalizePath(upload.path);

        std::shared_ptr<M3Render> existingRender;
        {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            auto it = mModelCache.find(normalized);
            if (it != mModelCache.end() && it->second.valid)
                existingRender = it->second.render;
        }

        if (existingRender)
        {
            PropDebugLog("Using cached render for: " + normalized);
            upload.prop->render = existingRender;
            std::atomic_thread_fence(std::memory_order_release);
            upload.prop->loaded.store(true, std::memory_order_release);
            delete upload.modelData;
            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
            processed++;
            continue;
        }

        std::shared_ptr<M3Render> render;
        try
        {
            PropDebugLog("Creating M3Render for: " + upload.path);
            render = std::make_shared<M3Render>(*upload.modelData, archive, true, true);
            render->setModelName(upload.path);

            render->queueTexturesForLoading();

            bool hasPending = render->hasPendingTextures();
            PropDebugLog("  M3Render created, has pending textures: " + std::string(hasPending ? "YES" : "NO"));
        }
        catch (const std::exception& e)
        {
            PropDebugLogError("Exception creating M3Render for " + upload.path + ": " + e.what());
            delete upload.modelData;
            upload.prop->loaded.store(true, std::memory_order_release);
            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
            processed++;
            continue;
        }
        catch (...)
        {
            PropDebugLogError("Unknown exception creating M3Render for " + upload.path);
            delete upload.modelData;
            upload.prop->loaded.store(true, std::memory_order_release);
            mPendingCount.fetch_sub(1, std::memory_order_relaxed);
            processed++;
            continue;
        }

        CacheModel(upload.path, render);

        upload.prop->render = render;
        std::atomic_thread_fence(std::memory_order_release);
        upload.prop->loaded.store(true, std::memory_order_release);

        delete upload.modelData;
        mPendingCount.fetch_sub(1, std::memory_order_relaxed);
        processed++;
    }

    int texturesUploaded = 0;
    int maxTexturesPerFrame = 50;

    if (archive)
    {
        std::vector<std::shared_ptr<M3Render>> rendersToProcess;
        std::vector<std::string> renderNames;
        {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            rendersToProcess.reserve(mModelCache.size());
            for (auto& [path, cached] : mModelCache)
            {
                if (cached.valid && cached.render && cached.render->hasPendingTextures())
                {
                    rendersToProcess.push_back(cached.render);
                    renderNames.push_back(path);
                }
            }
        }

        if (!rendersToProcess.empty())
        {
            PropDebugLog("Processing textures for " + std::to_string(rendersToProcess.size()) + " models with pending textures");
        }

        for (size_t ri = 0; ri < rendersToProcess.size(); ri++)
        {
            auto& render = rendersToProcess[ri];
            const std::string& modelName = renderNames[ri];

            int texturesForThisModel = 0;
            while (render->hasPendingTextures() && texturesUploaded < maxTexturesPerFrame)
            {
                render->uploadNextTexture(archive);
                texturesUploaded++;
                texturesForThisModel++;
            }

            if (texturesForThisModel > 0)
            {
                PropDebugLog("  Uploaded " + std::to_string(texturesForThisModel) + " textures for: " + modelName +
                             " (still pending: " + std::string(render->hasPendingTextures() ? "YES" : "NO") + ")");
            }

            if (texturesUploaded >= maxTexturesPerFrame)
                break;
        }

        if (texturesUploaded > 0)
        {
            PropDebugLog("Uploaded " + std::to_string(texturesUploaded) + " textures this frame");
        }
    }
    else
    {
        // Check if there are pending textures but no archive
        std::lock_guard<std::mutex> lock(mCacheMutex);
        for (auto& [path, cached] : mModelCache)
        {
            if (cached.valid && cached.render && cached.render->hasPendingTextures())
            {
                PropDebugLogError("Archive is null but model has pending textures: " + path);
                break;
            }
        }
    }
}

bool PropLoader::HasPendingWork() const
{
    if (mPendingCount.load(std::memory_order_relaxed) > 0)
        return true;

    std::lock_guard<std::mutex> lock(mCacheMutex);
    for (const auto& [path, cached] : mModelCache)
    {
        if (cached.valid && cached.render && cached.render->hasPendingTextures())
            return true;
    }
    return false;
}

size_t PropLoader::GetPendingCount() const
{
    return mPendingCount.load(std::memory_order_relaxed);
}