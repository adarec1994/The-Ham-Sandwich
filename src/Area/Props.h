#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class M3Render;
class Archive;
class FileEntry;
struct M3ModelData;

using ArchivePtr = std::shared_ptr<Archive>;
using FileEntryPtr = std::shared_ptr<FileEntry>;

#pragma pack(push, 1)
struct PropPlacement
{
    uint16_t minX;
    uint16_t minY;
    uint16_t maxX;
    uint16_t maxY;
};

struct PropColor
{
    uint8_t r, g, b, a;
};
#pragma pack(pop)

enum class PropModelType : int32_t
{
    M3 = 0,
    I3 = 1,
    Unk_2 = 2,
    DGN = 3,
    Unk_4 = 4
};

struct Prop
{
    uint32_t uniqueID = 0;
    uint32_t someID = 0;
    int32_t unk0 = 0;
    int32_t unk1 = 0;
    PropModelType modelType = PropModelType::M3;
    int32_t nameOffset = 0;
    int32_t unkOffset = 0;
    float scale = 1.0f;
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f);
    PropPlacement placement = {0, 0, 3000, 3000};
    int32_t unk7 = 0;
    int32_t unk8 = 0;
    int32_t unk9 = 0;
    PropColor color0 = {255, 255, 255, 255};
    PropColor color1 = {255, 255, 255, 255};
    int32_t unk10 = 0;
    int32_t unk11 = 0;
    PropColor color2 = {255, 255, 255, 255};
    int32_t unk12 = 0;
    std::string path;

    std::atomic<bool> loadRequested{false};
    std::atomic<bool> loaded{false};
    std::shared_ptr<M3Render> render;

    Prop() = default;

    Prop(const Prop& other)
        : uniqueID(other.uniqueID)
        , someID(other.someID)
        , unk0(other.unk0)
        , unk1(other.unk1)
        , modelType(other.modelType)
        , nameOffset(other.nameOffset)
        , unkOffset(other.unkOffset)
        , scale(other.scale)
        , rotation(other.rotation)
        , position(other.position)
        , placement(other.placement)
        , unk7(other.unk7)
        , unk8(other.unk8)
        , unk9(other.unk9)
        , color0(other.color0)
        , color1(other.color1)
        , unk10(other.unk10)
        , unk11(other.unk11)
        , color2(other.color2)
        , unk12(other.unk12)
        , path(other.path)
        , loadRequested(other.loadRequested.load(std::memory_order_relaxed))
        , loaded(other.loaded.load(std::memory_order_relaxed))
        , render(other.render)
    {}

    Prop(Prop&& other) noexcept
        : uniqueID(other.uniqueID)
        , someID(other.someID)
        , unk0(other.unk0)
        , unk1(other.unk1)
        , modelType(other.modelType)
        , nameOffset(other.nameOffset)
        , unkOffset(other.unkOffset)
        , scale(other.scale)
        , rotation(other.rotation)
        , position(other.position)
        , placement(other.placement)
        , unk7(other.unk7)
        , unk8(other.unk8)
        , unk9(other.unk9)
        , color0(other.color0)
        , color1(other.color1)
        , unk10(other.unk10)
        , unk11(other.unk11)
        , color2(other.color2)
        , unk12(other.unk12)
        , path(std::move(other.path))
        , loadRequested(other.loadRequested.load(std::memory_order_relaxed))
        , loaded(other.loaded.load(std::memory_order_relaxed))
        , render(std::move(other.render))
    {}

    Prop& operator=(const Prop& other)
    {
        if (this != &other)
        {
            uniqueID = other.uniqueID;
            someID = other.someID;
            unk0 = other.unk0;
            unk1 = other.unk1;
            modelType = other.modelType;
            nameOffset = other.nameOffset;
            unkOffset = other.unkOffset;
            scale = other.scale;
            rotation = other.rotation;
            position = other.position;
            placement = other.placement;
            unk7 = other.unk7;
            unk8 = other.unk8;
            unk9 = other.unk9;
            color0 = other.color0;
            color1 = other.color1;
            unk10 = other.unk10;
            unk11 = other.unk11;
            color2 = other.color2;
            unk12 = other.unk12;
            path = other.path;
            loadRequested.store(other.loadRequested.load(std::memory_order_relaxed), std::memory_order_relaxed);
            loaded.store(other.loaded.load(std::memory_order_relaxed), std::memory_order_relaxed);
            render = other.render;
        }
        return *this;
    }

    Prop& operator=(Prop&& other) noexcept
    {
        if (this != &other)
        {
            uniqueID = other.uniqueID;
            someID = other.someID;
            unk0 = other.unk0;
            unk1 = other.unk1;
            modelType = other.modelType;
            nameOffset = other.nameOffset;
            unkOffset = other.unkOffset;
            scale = other.scale;
            rotation = other.rotation;
            position = other.position;
            placement = other.placement;
            unk7 = other.unk7;
            unk8 = other.unk8;
            unk9 = other.unk9;
            color0 = other.color0;
            color1 = other.color1;
            unk10 = other.unk10;
            unk11 = other.unk11;
            color2 = other.color2;
            unk12 = other.unk12;
            path = std::move(other.path);
            loadRequested.store(other.loadRequested.load(std::memory_order_relaxed), std::memory_order_relaxed);
            loaded.store(other.loaded.load(std::memory_order_relaxed), std::memory_order_relaxed);
            render = std::move(other.render);
        }
        return *this;
    }
};

struct CurtData
{
    int32_t unk0 = 0;
    int16_t positionCount = 0;
    PropPlacement placement;
    int16_t unk5 = 0;
    int32_t positionOffset = 0;
    int32_t unk6 = 0;
    std::vector<glm::vec3> positions;
};

std::string ReadWideString(const uint8_t* data, size_t offset, size_t maxLen = 512);
void ParsePropsChunk(const uint8_t* data, size_t size, std::vector<Prop>& outProps, std::unordered_map<uint32_t, size_t>& outLookup);
void ParseCurtsChunk(const uint8_t* data, size_t size, std::vector<CurtData>& outCurts);

struct PropLoadResult
{
    uint32_t uniqueID = 0;
    std::string path;
    std::vector<uint8_t> fileData;
    M3ModelData* modelData = nullptr;
    bool success = false;
    bool fromCache = false;
};

struct CachedModel
{
    std::shared_ptr<M3Render> render;
    bool valid = false;
};

struct DecodedTexture
{
    std::vector<uint8_t> rgba;
    int width = 0;
    int height = 0;
    bool valid = false;
    bool uploaded = false;
};

struct PreloadedModel
{
    std::unique_ptr<M3ModelData> data;
    bool valid = false;
};

class PropLoader
{
public:
    static PropLoader& Instance();

    void Initialize(size_t numThreads = 0);
    void Shutdown();

    void SetArchive(const ArchivePtr& archive);

    void QueueProp(Prop* prop);
    void QueueProps(std::vector<Prop*>& props);

    void ProcessGPUUploads(int maxPerFrame = 10);

    void ClearPendingWork();

    bool HasPendingWork() const;
    size_t GetPendingCount() const;
    size_t GetCacheSize() const;

    std::shared_ptr<M3Render> GetCachedModel(const std::string& path);
    void CacheModel(const std::string& path, std::shared_ptr<M3Render> render);

    void ClearCache();

    void PreloadTexture(const std::string& path);
    void PreloadTextures(const std::vector<std::string>& paths);

    DecodedTexture* GetDecodedTexture(const std::string& path);

    unsigned int UploadDecodedTexture(const DecodedTexture& tex);

    void PreloadModel(const std::string& path);
    void PreloadModels(const std::vector<std::string>& paths);

    PreloadedModel* GetPreloadedModel(const std::string& path);

private:
    PropLoader() = default;
    ~PropLoader();

    PropLoader(const PropLoader&) = delete;
    PropLoader& operator=(const PropLoader&) = delete;

    void WorkerThread();
    PropLoadResult LoadPropData(Prop* prop);
    FileEntryPtr FindPropFile(const std::string& path);

    ArchivePtr mArchive;
    mutable std::mutex mArchiveMutex;

    std::vector<std::thread> mWorkers;
    std::queue<Prop*> mWorkQueue;
    mutable std::mutex mWorkMutex;
    std::condition_variable mWorkCondition;
    std::atomic<bool> mRunning{false};

    struct PendingUpload
    {
        Prop* prop = nullptr;
        M3ModelData* modelData = nullptr;
        std::string path;
    };
    std::queue<PendingUpload> mPendingUploads;
    mutable std::mutex mUploadMutex;

    std::unordered_map<std::string, CachedModel> mModelCache;
    mutable std::mutex mCacheMutex;

    std::unordered_map<std::string, FileEntryPtr> mFileCache;
    mutable std::mutex mFileCacheMutex;

    std::mutex mArchiveAccessMutex;

    std::atomic<size_t> mPendingCount{0};

    std::unordered_map<std::string, DecodedTexture> mDecodedTextures;
    mutable std::mutex mDecodedTextureMutex;

    std::unordered_map<std::string, PreloadedModel> mPreloadedModels;
    mutable std::mutex mPreloadedModelMutex;

    std::queue<std::string> mTextureDecodeQueue;
    mutable std::mutex mTextureQueueMutex;

    std::queue<std::string> mModelPreloadQueue;
    mutable std::mutex mModelQueueMutex;

    void DecodeTextureInWorker(const std::string& path);
    void PreloadModelInWorker(const std::string& path);

    ArchivePtr getArchive() const
    {
        std::lock_guard<std::mutex> lock(mArchiveMutex);
        return mArchive;
    }
};

void PrintPropTextureDebugSummary();
void ResetPropTextureDebugCounters();
void SetPropTextureDebugEnabled(bool enabled);
void PrintFailedTextures();
void RecordTextureFailure(const std::string& modelPath, const std::string& texturePath);