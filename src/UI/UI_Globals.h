#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <atomic>
#include <glm/glm.hpp>
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"
#include "../Archive.h"

extern glm::mat4 gViewMatrix;
extern glm::mat4 gProjMatrix;

extern std::vector<AreaFilePtr> gLoadedAreas;

extern AreaChunkRenderPtr gSelectedChunk;
extern int gSelectedChunkIndex;
extern int gSelectedAreaIndex;
extern std::string gSelectedChunkAreaName;

extern std::shared_ptr<M3Render> gLoadedModel;

extern bool gIsLoadingModel;
extern std::string gLoadingModelName;
extern ArchivePtr gPendingModelArchive;
extern std::shared_ptr<FileEntry> gPendingModelFile;

void ProcessModelLoading(AppState& state);
void StartLoadingModel(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& file, const std::string& name);


extern bool gIsLoadingAreas;
extern int gLoadingAreasCurrent;
extern int gLoadingAreasTotal;
extern std::string gLoadingAreasName;
extern std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> gPendingAreaFiles;


void ProcessAreaLoading(AppState& state);


void StartLoadingAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry);


extern std::atomic<bool> gIsDumping;
extern bool gShowDumpFolderDialog;
extern std::atomic<int> gDumpCurrent;
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