#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

class AreaFile;
using AreaFilePtr = std::shared_ptr<AreaFile>;

namespace GLBExport
{
    using ProgressCallback = std::function<void(int current, int total, const std::string& status)>;

    struct ExportSettings
    {
        std::string outputPath;
        float scale = 1.0f;
        bool exportTextures = true;
        int textureSize = 256;
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
    };

    ExportResult ExportAreaToGLB(const AreaFilePtr& area, const ExportSettings& settings, ProgressCallback progress = nullptr);
    ExportResult ExportAreasToGLB(const std::vector<AreaFilePtr>& areas, const ExportSettings& settings, ProgressCallback progress = nullptr);
    std::string GetSuggestedFilename(const AreaFilePtr& area);
}