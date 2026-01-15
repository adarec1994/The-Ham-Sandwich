// src/UI/UI_Globals.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"
#include "../Archive.h"

extern std::vector<AreaFilePtr> gLoadedAreas;

extern AreaChunkRenderPtr gSelectedChunk;
extern int gSelectedChunkIndex;
extern int gSelectedAreaIndex;
extern std::string gSelectedChunkAreaName;

extern std::shared_ptr<M3Render> gLoadedModel;

// Loading state
extern bool gIsLoadingAreas;
extern int gLoadingAreasCurrent;
extern int gLoadingAreasTotal;
extern std::string gLoadingAreasName;
extern std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> gPendingAreaFiles;

// Call this each frame to process pending area loads
void ProcessAreaLoading(AppState& state);

// Start loading areas from a folder
void StartLoadingAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry);