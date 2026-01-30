#pragma once

#include "../Archive.h"
#include <string>
#include <vector>
#include <memory>
#include <utility>

struct AppState;

namespace Heightmap {

    bool GenerateCompositeThumbnailFromFiles(
        const std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>>& areaFiles,
        std::vector<uint8_t>& outPixels, int& outWidth, int& outHeight,
        uint64_t generation);

    bool IsCompositeHeightmapGenerating();
    float GetCompositeHeightmapProgress();
    bool IsCompositeHeightmapReady(uint64_t currentGeneration);
    void ClearCompositeHeightmapCache();

    void SetPendingSingleHeightmapExport(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry, const std::string& name);
    void SetPendingAllHeightmapExport(const std::string& folderName);

    void GenerateSingleAreaHeightmap(const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry, const std::string& name);
    void GenerateAllAreasHeightmap(const std::string& folderName, uint64_t currentGeneration);

    void ViewSingleAreaHeightmap(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry, const std::string& name);
    void ViewAllAreasHeightmap(AppState& state, const std::string& folderName, uint64_t currentGeneration);

    void ExportHeightmapToFile(const std::string& path, const std::string& format);

    void DrawHeightmapViewer(uint64_t currentGeneration);

}