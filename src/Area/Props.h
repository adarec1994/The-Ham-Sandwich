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

    bool loadRequested = false;
    bool loaded = false;
    std::shared_ptr<M3Render> render;
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

    bool HasPendingWork() const;
    size_t GetPendingCount() const;
    size_t GetCacheSize() const;

    std::shared_ptr<M3Render> GetCachedModel(const std::string& path);
    void CacheModel(const std::string& path, std::shared_ptr<M3Render> render);

    void ClearCache();

private:
    PropLoader() = default;
    ~PropLoader();

    PropLoader(const PropLoader&) = delete;
    PropLoader& operator=(const PropLoader&) = delete;

    void WorkerThread();
    PropLoadResult LoadPropData(Prop* prop);
    FileEntryPtr FindPropFile(const std::string& path);

    ArchivePtr mArchive;

    std::vector<std::thread> mWorkers;
    std::queue<Prop*> mWorkQueue;
    std::mutex mWorkMutex;
    std::condition_variable mWorkCondition;
    std::atomic<bool> mRunning{false};

    struct PendingUpload
    {
        Prop* prop = nullptr;
        M3ModelData* modelData = nullptr;
        std::string path;
    };
    std::queue<PendingUpload> mPendingUploads;
    std::mutex mUploadMutex;

    std::unordered_map<std::string, CachedModel> mModelCache;
    std::mutex mCacheMutex;

    std::unordered_map<std::string, FileEntryPtr> mFileCache;
    std::mutex mFileCacheMutex;

    std::atomic<size_t> mPendingCount{0};
};