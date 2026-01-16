#include "ParallelAreaLoader.h"
#include <chrono>
#include <thread>

ParallelAreaLoader::~ParallelAreaLoader()
{
    cancel();
    for (auto& f : mParseFutures)
    {
        if (f.valid()) f.wait();
    }
}

void ParallelAreaLoader::loadAreas(
    const ArchivePtr& archive,
    const std::vector<LoadRequest>& requests,
    ProgressCallback progress)
{
    if (requests.empty()) return;
    
    mArchive = archive;
    mProgressCallback = progress;
    mTotalCount = static_cast<int>(requests.size());
    mCompletedParseCount = 0;
    mCancelled = false;
    mResults.clear();
    mParseFutures.clear();
    
    for (const auto& req : requests)
    {
        mParseFutures.push_back(std::async(std::launch::async, [this, archive, req]() {
            if (mCancelled) return;
            
            auto parsed = AreaFile::parseAreaFile(archive, req.file);
            
            if (mCancelled) return;
            
            {
                std::lock_guard<std::mutex> lock(mUploadMutex);
                mPendingUploads.push(std::move(parsed));
            }
            
            int completed = ++mCompletedParseCount;
            if (mProgressCallback)
            {
                mProgressCallback(completed, mTotalCount);
            }
        }));
    }
}

std::vector<AreaFilePtr> ParallelAreaLoader::loadAreasSync(
    const ArchivePtr& archive,
    const std::vector<LoadRequest>& requests,
    ProgressCallback progress)
{
    loadAreas(archive, requests, progress);
    
    for (auto& f : mParseFutures)
    {
        if (f.valid()) f.wait();
    }
    
    while (getPendingGPUUploads() > 0)
    {
        processGPUUploads(0);
    }
    
    return getResults();
}

int ParallelAreaLoader::processGPUUploads(float timeLimitMs)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    int processed = 0;
    
    while (true)
    {
        ParsedArea parsed;
        {
            std::lock_guard<std::mutex> lock(mUploadMutex);
            if (mPendingUploads.empty()) break;
            parsed = std::move(mPendingUploads.front());
            mPendingUploads.pop();
        }
        
        auto area = uploadToGPU(mArchive, std::move(parsed));
        if (area)
        {
            std::lock_guard<std::mutex> lock(mResultsMutex);
            mResults.push_back(area);
        }
        
        processed++;
        
        if (timeLimitMs > 0)
        {
            auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
            float ms = std::chrono::duration<float, std::milli>(elapsed).count();
            if (ms >= timeLimitMs) break;
        }
    }
    
    return processed;
}

AreaFilePtr ParallelAreaLoader::uploadToGPU(const ArchivePtr& archive, ParsedArea&& parsed)
{
    if (!parsed.valid) return nullptr;
    
    auto area = std::make_shared<AreaFile>(archive, parsed.file);
    area->loadFromParsed(std::move(parsed));
    
    return area;
}

bool ParallelAreaLoader::isComplete() const
{
    return mCompletedParseCount.load() >= mTotalCount && getPendingGPUUploads() == 0;
}

std::vector<AreaFilePtr> ParallelAreaLoader::getResults()
{
    std::lock_guard<std::mutex> lock(mResultsMutex);
    return mResults;
}

void ParallelAreaLoader::cancel()
{
    mCancelled = true;
}

int ParallelAreaLoader::getPendingGPUUploads() const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mUploadMutex));
    return static_cast<int>(mPendingUploads.size());
}

std::vector<AreaFilePtr> LoadAreasParallel(
    const ArchivePtr& archive,
    const std::vector<FileEntryPtr>& files,
    std::function<void(int, int)> progress)
{
    std::vector<ParallelAreaLoader::LoadRequest> requests;
    requests.reserve(files.size());
    
    for (const auto& f : files)
    {
        requests.push_back({f, -1, -1});
    }
    
    ParallelAreaLoader loader;
    return loader.loadAreasSync(archive, requests, progress);
}