#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

class AreaFile;
class AreaChunkRender;
class Archive;

using AreaFilePtr = std::shared_ptr<AreaFile>;
using AreaChunkRenderPtr = std::shared_ptr<AreaChunkRender>;
using ArchivePtr = std::shared_ptr<Archive>;

namespace FBXExport
{
    using ProgressCallback = std::function<void(int, int, const std::string&)>;

    struct ExportSettings
    {
        bool combineMeshes = true;
        bool exportNormals = true;
        bool exportUVs = true;
        bool exportTextures = true;
        float scale = 1.0f;
        std::string outputPath;
    };

    struct ExportResult
    {
        bool success = false;
        std::string errorMessage;
        std::string outputFile;
        int vertexCount = 0;
        int triangleCount = 0;
        int chunkCount = 0;
        int textureCount = 0;
    };

    ExportResult ExportAreaToFBX(const AreaFilePtr& area, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress = nullptr);
    ExportResult ExportAreasToFBX(const std::vector<AreaFilePtr>& areas, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress = nullptr);
    std::string GetSuggestedFilename(const AreaFilePtr& area);
}