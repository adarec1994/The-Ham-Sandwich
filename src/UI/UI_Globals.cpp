// src/UI/UI_Globals.cpp
#include "UI_Globals.h"
#include "UI.h"
#include "UI_Utils.h"
#include "../Area/AreaFile.h"
#include <iostream>

std::vector<AreaFilePtr> gLoadedAreas;

AreaChunkRenderPtr gSelectedChunk = nullptr;
int gSelectedChunkIndex = -1;
int gSelectedAreaIndex = -1;
std::string gSelectedChunkAreaName;

std::shared_ptr<M3Render> gLoadedModel = nullptr;

// Loading state
bool gIsLoadingAreas = false;
int gLoadingAreasCurrent = 0;
int gLoadingAreasTotal = 0;
std::string gLoadingAreasName;
std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> gPendingAreaFiles;

extern void SnapCameraToLoaded(AppState& state);

void StartLoadingAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry)
{
    if (!arc || !folderEntry || !folderEntry->isDirectory()) return;

    // Reset reference position so first area renders at origin
    ResetAreaReferencePosition();

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
    gLoadedModel = nullptr;
    gPendingAreaFiles.clear();

    // Collect all .area files
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

    // Load a few areas per frame to keep UI responsive
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

    // Check if done
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