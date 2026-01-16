#pragma once

#include <vector>
#include <memory>
#include <future>
#include <mutex>
#include <functional>
#include <atomic>
#include <queue>
#include "../Area/AreaFile.h"

class ParallelAreaLoader
{
public:
    struct LoadRequest
    {
        FileEntryPtr file;
        int tileX = -1;
        int tileY = -1;
    };
    
    using ProgressCallback = std::function<void(int completed, int total)>;
    
    ParallelAreaLoader() = default;
    ~ParallelAreaLoader();
    
    void loadAreas(
        const ArchivePtr& archive,
        const std::vector<LoadRequest>& requests,
        ProgressCallback progress = nullptr);
    
    std::vector<AreaFilePtr> loadAreasSync(
        const ArchivePtr& archive,
        const std::vector<LoadRequest>& requests,
        ProgressCallback progress = nullptr);
    
    int processGPUUploads(float timeLimitMs = 16.0f);
    
    bool isComplete() const;
    std::vector<AreaFilePtr> getResults();
    void cancel();
    
    int getTotalCount() const { return mTotalCount; }
    int getCompletedCount() const { return mCompletedParseCount.load(); }
    int getPendingGPUUploads() const;
    
private:
    AreaFilePtr uploadToGPU(const ArchivePtr& archive, ParsedArea&& parsed);
    
    std::atomic<int> mCompletedParseCount{0};
    std::atomic<bool> mCancelled{false};
    int mTotalCount = 0;
    
    std::queue<ParsedArea> mPendingUploads;
    std::mutex mUploadMutex;
    
    std::vector<AreaFilePtr> mResults;
    std::mutex mResultsMutex;
    
    ArchivePtr mArchive;
    ProgressCallback mProgressCallback;
    std::vector<std::future<void>> mParseFutures;
};

std::vector<AreaFilePtr> LoadAreasParallel(
    const ArchivePtr& archive,
    const std::vector<FileEntryPtr>& files,
    std::function<void(int, int)> progress = nullptr);