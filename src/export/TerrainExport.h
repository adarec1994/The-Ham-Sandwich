#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>

class AreaFile;
using AreaFilePtr = std::shared_ptr<AreaFile>;

namespace TerrainExport
{
    using ProgressCallback = std::function<void(int current, int total, const std::string& status)>;

    struct ExportSettings
    {
        std::string outputPath;
        std::string sharedModelsPath;
        float scale = 1.0f;
        bool exportProps = true;
        bool exportSkybox = true;
        bool exportPropModels = true;
        std::set<std::string>* exportedModels = nullptr;
    };

    struct ExportResult
    {
        bool success = false;
        std::string outputFile;
        std::string errorMessage;
        int vertexCount = 0;
        int triangleCount = 0;
        int chunkCount = 0;
        int textureCount = 0;
        int propCount = 0;
        int skyboxCount = 0;
    };

    ExportResult ExportAreaToTerrain(const AreaFilePtr& area, const ExportSettings& settings, ProgressCallback progress = nullptr);
    ExportResult ExportAreasToTerrain(const std::vector<AreaFilePtr>& areas, const ExportSettings& settings, ProgressCallback progress = nullptr);
    std::string GetSuggestedFilename(const AreaFilePtr& area);
}