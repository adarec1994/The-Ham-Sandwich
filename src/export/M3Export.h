#pragma once

#include "../models/M3Common.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

class M3Render;
class Archive;
using ArchivePtr = std::shared_ptr<Archive>;

namespace M3Export
{
    using ProgressCallback = std::function<void(int current, int total, const std::string& status)>;

    struct ExportSettings
    {
        std::string outputPath;
        float scale = 1.0f;
        bool exportTextures = true;
        bool exportAnimations = true;
        bool exportSkeleton = true;
        int activeVariant = -1;
    };

    struct ExportResult
    {
        bool success = false;
        std::string outputFile;
        std::string errorMessage;
        int vertexCount = 0;
        int triangleCount = 0;
        int boneCount = 0;
        int animationCount = 0;
        int textureCount = 0;
    };

    ExportResult ExportToFBX(M3Render* render, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress = nullptr);
    ExportResult ExportToGLB(M3Render* render, const ArchivePtr& archive, const ExportSettings& settings, ProgressCallback progress = nullptr);
    std::string GetSuggestedFilename(M3Render* render);
}