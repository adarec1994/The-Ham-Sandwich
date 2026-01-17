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


extern bool gIsLoadingAreas;
extern int gLoadingAreasCurrent;
extern int gLoadingAreasTotal;
extern std::string gLoadingAreasName;
extern std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> gPendingAreaFiles;


void ProcessAreaLoading(AppState& state);


void StartLoadingAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry);


extern bool gIsDumping;
extern bool gShowDumpFolderDialog;
extern int gDumpCurrent;
extern int gDumpTotal;
extern std::string gDumpCurrentFile;
extern std::string gDumpOutputPath;

struct DumpEntry {
    ArchivePtr arc;
    std::shared_ptr<FileEntry> file;
    std::wstring relativePath;
};
extern std::vector<DumpEntry> gPendingDumpFiles;

void StartDumpAll(const std::vector<ArchivePtr>& archives, const std::string& outputPath);
void ProcessDumping();