#include "UI_Globals.h"
#include "UI.h"
#include "UI_Utils.h"
#include "../Area/AreaFile.h"
#include <iostream>
#include <fstream>
#include <filesystem>

std::vector<AreaFilePtr> gLoadedAreas;

AreaChunkRenderPtr gSelectedChunk = nullptr;
int gSelectedChunkIndex = -1;
int gSelectedAreaIndex = -1;
std::string gSelectedChunkAreaName;

std::shared_ptr<M3Render> gLoadedModel = nullptr;

bool gIsLoadingAreas = false;
int gLoadingAreasCurrent = 0;
int gLoadingAreasTotal = 0;
std::string gLoadingAreasName;
std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> gPendingAreaFiles;

bool gIsDumping = false;
bool gShowDumpFolderDialog = false;
int gDumpCurrent = 0;
int gDumpTotal = 0;
std::string gDumpCurrentFile;
std::string gDumpOutputPath;
std::vector<DumpEntry> gPendingDumpFiles;

extern void SnapCameraToLoaded(AppState& state);

void StartLoadingAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry)
{
    if (!arc || !folderEntry || !folderEntry->isDirectory()) return;

    ResetAreaReferencePosition();

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
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
}

void ProcessDumping()
{
    if (!gIsDumping || gPendingDumpFiles.empty()) return;

    int dumpPerFrame = 50;
    for (int i = 0; i < dumpPerFrame && !gPendingDumpFiles.empty(); ++i)
    {
        auto entry = gPendingDumpFiles.back();
        gPendingDumpFiles.pop_back();

        std::string relPath = wstring_to_utf8(entry.relativePath);
        gDumpCurrentFile = relPath;

        std::filesystem::path outPath = std::filesystem::path(gDumpOutputPath) / relPath;

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
        catch (const std::exception& e)
        {
            std::cerr << "Failed to dump: " << relPath << " - " << e.what() << std::endl;
        }

        gDumpCurrent++;
    }

    if (gPendingDumpFiles.empty())
    {
        gIsDumping = false;
        gDumpCurrentFile = "";
        std::cout << "Dump complete: " << gDumpCurrent << " files" << std::endl;
    }
}