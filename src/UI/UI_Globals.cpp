#define NOMINMAX
#include "UI_Globals.h"
#include "UI.h"
#include "UI_Utils.h"
#include "../Area/AreaFile.h"
#include "../models/M3Loader.h"
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <cfloat>

glm::mat4 gViewMatrix = glm::mat4(1.0f);
glm::mat4 gProjMatrix = glm::mat4(1.0f);

std::vector<AreaFilePtr> gLoadedAreas;

AreaChunkRenderPtr gSelectedChunk = nullptr;
int gSelectedChunkIndex = -1;
int gSelectedAreaIndex = -1;
std::string gSelectedAreaName;

bool gShowProps = true;

std::shared_ptr<M3Render> gLoadedModel = nullptr;

bool gIsLoadingModel = false;
std::string gLoadingModelName;
ArchivePtr gPendingModelArchive = nullptr;
std::shared_ptr<FileEntry> gPendingModelFile = nullptr;
static std::atomic<bool> gModelLoadComplete{false};
static M3ModelData gLoadedModelData;
static std::thread gModelLoadThread;

bool gIsLoadingAreas = false;
int gLoadingAreasCurrent = 0;
int gLoadingAreasTotal = 0;
std::string gLoadingAreasName;
std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> gPendingAreaFiles;

std::atomic<bool> gIsDumping{false};
bool gShowDumpFolderDialog = false;
std::atomic<int> gDumpCurrent{0};
int gDumpTotal = 0;
std::string gDumpCurrentFile;
std::string gDumpOutputPath;
std::vector<DumpEntry> gPendingDumpFiles;
static std::vector<std::thread> gDumpThreads;
static std::mutex gDumpMutex;

extern void SnapCameraToLoaded(AppState& state);
extern void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax);

void StartLoadingModel(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& file, const std::string& name)
{
    if (gIsLoadingModel) return;

    gPendingModelArchive = arc;
    gPendingModelFile = file;
    gLoadingModelName = name;
    gIsLoadingModel = true;
    gModelLoadComplete = false;
    gLoadedModelData = M3ModelData();

    gModelLoadThread = std::thread([arc, file]() {
        gLoadedModelData = M3Loader::LoadFromFile(arc, file);
        gModelLoadComplete = true;
    });
    gModelLoadThread.detach();
}

void ProcessModelLoading(AppState& state)
{
    if (!gIsLoadingModel) return;

    if (!gModelLoadComplete) return;

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
    gSelectedChunkIndex = -1;
    gSelectedAreaIndex = -1;
    gSelectedAreaName.clear();

    if (gLoadedModelData.success)
    {
        gLoadedModel = std::make_shared<M3Render>(gLoadedModelData, gPendingModelArchive);
        gLoadedModel->setModelName(gLoadingModelName);
        state.m3Render = gLoadedModel;
        state.show_models_window = true;

        const auto& verts = gLoadedModel->getVertices();
        if (!verts.empty())
        {
            glm::vec3 minB(FLT_MAX);
            glm::vec3 maxB(-FLT_MAX);
            for (const auto& v : verts)
            {
                minB.x = std::min(minB.x, v.position.x);
                minB.y = std::min(minB.y, v.position.y);
                minB.z = std::min(minB.z, v.position.z);
                maxB.x = std::max(maxB.x, v.position.x);
                maxB.y = std::max(maxB.y, v.position.y);
                maxB.z = std::max(maxB.z, v.position.z);
            }
            SnapCameraToModel(state, minB, maxB);
        }
        else
        {
            SnapCameraToModel(state, glm::vec3(-1.0f), glm::vec3(1.0f));
        }
    }
    else
    {
        gLoadedModel = nullptr;
        state.m3Render = nullptr;
        state.show_models_window = false;
    }

    gIsLoadingModel = false;
    gPendingModelFile = nullptr;
    gLoadingModelName.clear();
    gLoadedModelData = M3ModelData();
}

void StartLoadingAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry)
{
    if (!arc || !folderEntry || !folderEntry->isDirectory()) return;

    ResetAreaReferencePosition();

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
    gSelectedChunkIndex = -1;
    gSelectedAreaIndex = -1;
    gSelectedAreaName.clear();
    gLoadedModel = nullptr;
    gPendingAreaFiles.clear();

    for (const auto& child : folderEntry->getChildren())
    {
        if (!child || child->isDirectory()) continue;

        const std::string childName = wstring_to_utf8(child->getEntryName());
        if (!EndsWithNoCase(childName, ".area")) continue;

        const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(child);
        if (!fileEntry) continue;

        gPendingAreaFiles.push_back({arc, fileEntry});
    }

    if (gPendingAreaFiles.empty()) return;

    gIsLoadingAreas = true;
    gLoadingAreasCurrent = 0;
    gLoadingAreasTotal = static_cast<int>(gPendingAreaFiles.size());
    gLoadingAreasName = wstring_to_utf8(folderEntry->getEntryName());
}

void ProcessAreaLoading(AppState& state)
{
    if (!gIsLoadingAreas || gPendingAreaFiles.empty()) return;

    int loadPerFrame = 3;
    for (int i = 0; i < loadPerFrame && !gPendingAreaFiles.empty(); ++i)
    {
        auto& [arc, fileEntry] = gPendingAreaFiles.back();
        gPendingAreaFiles.pop_back();

        const auto af = std::make_shared<AreaFile>(arc, fileEntry);
        if (af->load())
        {
            gLoadedAreas.push_back(af);
        }

        gLoadingAreasCurrent++;
    }

    if (gPendingAreaFiles.empty())
    {
        gIsLoadingAreas = false;

        if (!gLoadedAreas.empty())
        {
            state.currentArea = gLoadedAreas.back();
            SnapCameraToLoaded(state);
        }
        else
        {
            state.currentArea.reset();
        }
    }
}

static void CollectFilesRecursive(const ArchivePtr& arc, const IFileSystemEntryPtr& entry, const std::wstring& currentPath)
{
    if (!entry) return;

    if (entry->isDirectory())
    {
        std::wstring dirPath = currentPath;
        if (!entry->getEntryName().empty())
        {
            if (!dirPath.empty()) dirPath += L"/";
            dirPath += entry->getEntryName();
        }

        for (const auto& child : entry->getChildren())
        {
            CollectFilesRecursive(arc, child, dirPath);
        }
    }
    else
    {
        auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry);
        if (fileEntry)
        {
            std::wstring filePath = currentPath;
            if (!filePath.empty()) filePath += L"/";
            filePath += entry->getEntryName();

            gPendingDumpFiles.push_back({arc, fileEntry, filePath});
        }
    }
}

void StartDumpAll(const std::vector<ArchivePtr>& archives, const std::string& outputPath)
{
    if (gIsDumping) return;

    gPendingDumpFiles.clear();
    gDumpOutputPath = outputPath;

    for (const auto& arc : archives)
    {
        if (!arc) continue;
        auto root = arc->getRoot();
        if (root)
        {
            CollectFilesRecursive(arc, root, L"");
        }
    }

    if (gPendingDumpFiles.empty()) return;

    gIsDumping = true;
    gDumpCurrent = 0;
    gDumpTotal = static_cast<int>(gPendingDumpFiles.size());
    gDumpCurrentFile = "";

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads < 2) numThreads = 2;
    if (numThreads > 16) numThreads = 16;

    gDumpThreads.clear();

    for (unsigned int t = 0; t < numThreads; ++t)
    {
        gDumpThreads.emplace_back([outputPath]() {
            while (true)
            {
                DumpEntry entry;
                {
                    std::lock_guard<std::mutex> lock(gDumpMutex);
                    if (gPendingDumpFiles.empty()) break;
                    entry = gPendingDumpFiles.back();
                    gPendingDumpFiles.pop_back();
                }

                std::string relPath = wstring_to_utf8(entry.relativePath);
                std::filesystem::path outPath = std::filesystem::path(outputPath) / relPath;

                try
                {
                    std::filesystem::create_directories(outPath.parent_path());

                    std::vector<uint8_t> data;
                    entry.arc->getFileData(entry.file, data);

                    if (!data.empty())
                    {
                        std::ofstream out(outPath, std::ios::binary);
                        if (out)
                        {
                            out.write(reinterpret_cast<const char*>(data.data()), data.size());
                        }
                    }
                }
                catch (const std::exception&)
                {
                }

                gDumpCurrent++;
            }
        });
    }

    std::thread convergenceThread([]() {
        for (auto& t : gDumpThreads)
        {
            if (t.joinable()) t.join();
        }
        gDumpThreads.clear();
        gIsDumping = false;
    });
    convergenceThread.detach();
}

void ProcessDumping()
{
}

ExtractContext gExtractContext;

void StartExtractSingle(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& file, const std::string& outputPath)
{
    if (!arc || !file || outputPath.empty()) return;

    std::vector<uint8_t> data;
    arc->getFileData(file, data);

    if (data.empty()) return;

    std::string fileName = wstring_to_utf8(file->getEntryName());
    std::filesystem::path outPath = std::filesystem::path(outputPath) / fileName;

    try
    {
        std::ofstream out(outPath, std::ios::binary);
        if (out)
        {
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }
    catch (...)
    {
    }
}

static void CollectFolderFilesRecursive(const ArchivePtr& arc, const IFileSystemEntryPtr& entry,
                                         const std::wstring& currentPath, std::vector<DumpEntry>& outFiles)
{
    if (!entry) return;

    if (entry->isDirectory())
    {
        std::wstring dirPath = currentPath;
        if (!entry->getEntryName().empty())
        {
            if (!dirPath.empty()) dirPath += L"/";
            dirPath += entry->getEntryName();
        }

        for (const auto& child : entry->getChildren())
        {
            CollectFolderFilesRecursive(arc, child, dirPath, outFiles);
        }
    }
    else
    {
        auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry);
        if (fileEntry)
        {
            std::wstring filePath = currentPath;
            if (!filePath.empty()) filePath += L"/";
            filePath += entry->getEntryName();

            outFiles.push_back({arc, fileEntry, filePath});
        }
    }
}

void StartExtractFolder(const ArchivePtr& arc, const IFileSystemEntryPtr& folder, const std::string& outputPath)
{
    if (!arc || !folder || !folder->isDirectory() || outputPath.empty()) return;

    std::vector<DumpEntry> filesToExtract;
    CollectFolderFilesRecursive(arc, folder, L"", filesToExtract);

    if (filesToExtract.empty()) return;

    std::string folderName = wstring_to_utf8(folder->getEntryName());
    std::filesystem::path basePath = std::filesystem::path(outputPath) / folderName;

    for (const auto& entry : filesToExtract)
    {
        std::string relPath = wstring_to_utf8(entry.relativePath);
        std::filesystem::path outPath = basePath / relPath;

        try
        {
            std::filesystem::create_directories(outPath.parent_path());

            std::vector<uint8_t> data;
            entry.arc->getFileData(entry.file, data);

            if (!data.empty())
            {
                std::ofstream out(outPath, std::ios::binary);
                if (out)
                {
                    out.write(reinterpret_cast<const char*>(data.data()), data.size());
                }
            }
        }
        catch (...)
        {
        }
    }
}