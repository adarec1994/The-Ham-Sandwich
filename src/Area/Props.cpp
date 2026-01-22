#define NOMINMAX
#include "Props.h"
#include "../Archive.h"
#include "../models/M3Loader.h"
#include "../models/M3LoaderV95.h"
#include "../models/M3Render.h"
#include <cstring>
#include <algorithm>
#include <iostream>

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
    if (mRunning) return;

    if (numThreads == 0)
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);

    mRunning = true;
    mWorkers.reserve(numThreads);

    for (size_t i = 0; i < numThreads; ++i)
        mWorkers.emplace_back(&PropLoader::WorkerThread, this);
}

void PropLoader::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mWorkMutex);
        mRunning = false;
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
    std::lock_guard<std::mutex> lock(mFileCacheMutex);
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
    return mModelCache.size();
}

FileEntryPtr PropLoader::FindPropFile(const std::string& path)
{
    if (!mArchive) return nullptr;

    std::string normalized = NormalizePath(path);

    {
        std::lock_guard<std::mutex> lock(mFileCacheMutex);
        auto it = mFileCache.find(normalized);
        if (it != mFileCache.end())
            return it->second;
    }

    FileEntryPtr fileEntry = mArchive->findFileCached(normalized);

    if (!fileEntry)
    {
        std::string withExt = normalized;
        if (withExt.find(".m3") == std::string::npos)
        {
            withExt += ".m3";
            fileEntry = mArchive->findFileCached(withExt);
        }
    }

    if (!fileEntry)
    {
        fileEntry = mArchive->findFileByNameCached(normalized);
        if (!fileEntry)
        {
            size_t lastSlash = normalized.rfind('/');
            if (lastSlash != std::string::npos)
            {
                std::string filename = normalized.substr(lastSlash + 1);
                fileEntry = mArchive->findFileByNameCached(filename);
                if (!fileEntry && filename.find(".m3") == std::string::npos)
                    fileEntry = mArchive->findFileByNameCached(filename + ".m3");
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
    if (!mRunning)
        Initialize();

    if (!prop || prop->loaded || prop->loadRequested)
        return;

    if (prop->modelType == PropModelType::Unk_2 || prop->modelType == PropModelType::Unk_4)
    {
        prop->loaded = true;
        return;
    }

    if (prop->path.empty())
    {
        prop->loaded = true;
        return;
    }

    std::string normalized = NormalizePath(prop->path);
    auto cached = GetCachedModel(normalized);
    if (cached)
    {
        prop->render = cached;
        prop->loaded = true;
        return;
    }

    prop->loadRequested = true;
    mPendingCount++;

    {
        std::lock_guard<std::mutex> lock(mWorkMutex);
        mWorkQueue.push(prop);
    }
    mWorkCondition.notify_one();
}

void PropLoader::QueueProps(std::vector<Prop*>& props)
{

    if (!mRunning)
    {
        std::cout << "[PropLoader] Auto-initializing..." << std::endl;
        Initialize();
    }

    std::vector<Prop*> toQueue;
    toQueue.reserve(props.size());

    for (Prop* prop : props)
    {
        if (!prop || prop->loaded || prop->loadRequested)
            continue;

        if (prop->modelType == PropModelType::Unk_2 || prop->modelType == PropModelType::Unk_4)
        {
            prop->loaded = true;
            continue;
        }

        if (prop->path.empty())
        {
            prop->loaded = true;
            continue;
        }

        std::string normalized = NormalizePath(prop->path);
        auto cached = GetCachedModel(normalized);
        if (cached)
        {
            prop->render = cached;
            prop->loaded = true;
            continue;
        }

        prop->loadRequested = true;
        toQueue.push_back(prop);
    }

    if (toQueue.empty()) return;

    mPendingCount += toQueue.size();

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
        result.success = false;
        return result;
    }

    std::vector<uint8_t> buffer;
    if (!mArchive->getFileData(fileEntry, buffer) || buffer.empty())
    {
        result.success = false;
        return result;
    }

    M3ModelData* modelData = new M3ModelData();

    if (buffer.size() >= 8)
    {
        uint32_t version = 0;
        std::memcpy(&version, buffer.data() + 4, sizeof(version));
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
                return !mWorkQueue.empty() || !mRunning;
            });

            if (!mRunning && mWorkQueue.empty())
                return;

            if (mWorkQueue.empty())
                continue;

            prop = mWorkQueue.front();
            mWorkQueue.pop();
        }

        if (!prop) continue;

        PropLoadResult result = LoadPropData(prop);

        if (result.fromCache)
        {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            std::string normalized = NormalizePath(prop->path);
            auto it = mModelCache.find(normalized);
            if (it != mModelCache.end() && it->second.valid)
            {
                prop->render = it->second.render;
                prop->loaded = true;
            }
            else
            {
                prop->loaded = true;
            }
            mPendingCount--;
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
            prop->loaded = true;
            mPendingCount--;
        }
    }
}

void PropLoader::ProcessGPUUploads(int maxPerFrame)
{
    int processed = 0;

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
            delete upload.modelData;
            mPendingCount--;
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
            upload.prop->render = existingRender;
            upload.prop->loaded = true;
            delete upload.modelData;
            mPendingCount--;
            processed++;
            continue;
        }


        M3ModelData dataNoTex = *upload.modelData;
        dataNoTex.textures.clear();

        auto render = std::make_shared<M3Render>(dataNoTex, mArchive);
        render->setModelName(upload.path);

        CacheModel(upload.path, render);

        upload.prop->render = render;
        upload.prop->loaded = true;

        delete upload.modelData;
        mPendingCount--;
        processed++;
    }
}

bool PropLoader::HasPendingWork() const
{
    return mPendingCount > 0;
}

size_t PropLoader::GetPendingCount() const
{
    return mPendingCount;
}