#include "UI_AreaInfo.h"
#include "UI_Globals.h"
#include "../export/FBXExport.h"
#include "../Area/AreaFile.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <glad/glad.h>
#include <filesystem>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

#include <stb_image_write.h>

namespace UI_AreaInfo
{
    static bool sShowWindow = true;
    static std::string sLastExportPath;
    static std::string sLastExportMessage;
    static bool sShowExportResult = false;
    static float sExportResultTimer = 0.0f;

    static FBXExport::ExportSettings sExportSettings;

    static std::atomic<bool> sExporting{false};
    static std::atomic<float> sExportProgress{0.0f};
    static std::atomic<int> sExportCurrentChunk{0};
    static std::atomic<int> sExportTotalChunks{0};
    static std::string sExportStatusText;
    static std::mutex sExportMutex;
    static FBXExport::ExportResult sExportResult;
    static bool sExportComplete = false;

    static bool sPendingExport = false;
    static std::string sPendingExportPath;
    static ArchivePtr sPendingArchive;

    static float sWindowBottomY = 50.0f;

    // Heightmap display state
    static bool sShowHeightmapWindow = false;
    static GLuint sHeightmapTexture = 0;
    static int sHeightmapWidth = 0;
    static int sHeightmapHeight = 0;
    static std::vector<uint8_t> sHeightmapData;
    static float sHeightmapMin = 0.0f;
    static float sHeightmapMax = 0.0f;
    static std::string sHeightmapExportMessage;
    static float sHeightmapExportTimer = 0.0f;

    static void GenerateHeightmapTexture()
    {
        if (gLoadedAreas.empty()) return;

        constexpr int AREA_SIZE = 256;

        int minTileX = std::numeric_limits<int>::max();
        int maxTileX = std::numeric_limits<int>::lowest();
        int minTileY = std::numeric_limits<int>::max();
        int maxTileY = std::numeric_limits<int>::lowest();

        for (const auto& area : gLoadedAreas)
        {
            if (!area) continue;
            minTileX = std::min(minTileX, area->getTileX());
            maxTileX = std::max(maxTileX, area->getTileX());
            minTileY = std::min(minTileY, area->getTileY());
            maxTileY = std::max(maxTileY, area->getTileY());
        }

        int tilesWide = maxTileX - minTileX + 1;
        int tilesHigh = maxTileY - minTileY + 1;
        int totalWidth = tilesWide * AREA_SIZE;
        int totalHeight = tilesHigh * AREA_SIZE;

        if (totalWidth > 4096 || totalHeight > 4096)
        {
            tilesWide = 1;
            tilesHigh = 1;
            totalWidth = AREA_SIZE;
            totalHeight = AREA_SIZE;
            minTileX = gLoadedAreas[0]->getTileX();
            minTileY = gLoadedAreas[0]->getTileY();
        }

        std::vector<float> heights(totalWidth * totalHeight, -100000.0f);
        float minHeight = std::numeric_limits<float>::max();
        float maxHeight = std::numeric_limits<float>::lowest();

        for (const auto& area : gLoadedAreas)
        {
            if (!area) continue;

            int areaTileX = area->getTileX() - minTileX;
            int areaTileY = area->getTileY() - minTileY;
            if (areaTileX < 0 || areaTileX >= tilesWide || areaTileY < 0 || areaTileY >= tilesHigh)
                continue;

            const auto& chunks = area->getChunks();

            for (int chunkIdx = 0; chunkIdx < static_cast<int>(chunks.size()); chunkIdx++)
            {
                const auto& chunk = chunks[chunkIdx];
                if (!chunk || !chunk->hasHeightmap()) continue;

                const auto& verts = chunk->getVertices();
                if (verts.empty()) continue;

                for (const auto& v : verts)
                {
                    int pixelX = static_cast<int>(v.x / 2.0f);
                    int pixelZ = static_cast<int>(v.z / 2.0f);

                    if (pixelX < 0 || pixelX >= AREA_SIZE || pixelZ < 0 || pixelZ >= AREA_SIZE)
                        continue;

                    int imgX = areaTileX * AREA_SIZE + pixelX;
                    int imgY = (tilesHigh - 1 - areaTileY) * AREA_SIZE + (AREA_SIZE - 1 - pixelZ);

                    if (imgX >= 0 && imgX < totalWidth && imgY >= 0 && imgY < totalHeight)
                    {
                        int idx = imgY * totalWidth + imgX;
                        heights[idx] = v.y;
                        minHeight = std::min(minHeight, v.y);
                        maxHeight = std::max(maxHeight, v.y);
                    }
                }
            }
        }

        sHeightmapMin = minHeight;
        sHeightmapMax = maxHeight;

        float range = maxHeight - minHeight;
        if (range < 0.001f) range = 1.0f;

        sHeightmapData.resize(totalWidth * totalHeight * 4);
        sHeightmapWidth = totalWidth;
        sHeightmapHeight = totalHeight;

        for (int i = 0; i < totalWidth * totalHeight; i++)
        {
            float h = heights[i];
            uint8_t val = 0;
            uint8_t alpha = 255;

            if (h > -99999.0f)
            {
                float normalized = (h - minHeight) / range;
                val = static_cast<uint8_t>(std::clamp(normalized * 255.0f, 0.0f, 255.0f));
            }
            else
            {
                val = 32;
                alpha = 128;
            }

            sHeightmapData[i * 4 + 0] = val;
            sHeightmapData[i * 4 + 1] = val;
            sHeightmapData[i * 4 + 2] = val;
            sHeightmapData[i * 4 + 3] = alpha;
        }

        if (sHeightmapTexture == 0)
        {
            glGenTextures(1, &sHeightmapTexture);
        }

        glBindTexture(GL_TEXTURE_2D, sHeightmapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, totalWidth, totalHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, sHeightmapData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static void ExportHeightmapPNG(const std::string& path)
    {
        if (sHeightmapData.empty() || sHeightmapWidth == 0 || sHeightmapHeight == 0)
        {
            sHeightmapExportMessage = "No heightmap data to export";
            sHeightmapExportTimer = 3.0f;
            return;
        }

        std::string fullPath = path;
        if (fullPath.find(".png") == std::string::npos && fullPath.find(".PNG") == std::string::npos)
        {
            fullPath += "/heightmap.png";
        }

        if (stbi_write_png(fullPath.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapData.data(), sHeightmapWidth * 4))
        {
            sHeightmapExportMessage = "Saved: " + fullPath;
        }
        else
        {
            sHeightmapExportMessage = "Failed to save heightmap";
        }
        sHeightmapExportTimer = 4.0f;
    }

    static void DrawHeightmapWindow()
    {
        if (!sShowHeightmapWindow) return;

        ImGui::SetNextWindowSize(ImVec2(540, 620), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Heightmap View", &sShowHeightmapWindow))
        {
            if (sHeightmapTexture != 0 && sHeightmapWidth > 0)
            {
                ImGui::Text("Resolution: %dx%d", sHeightmapWidth, sHeightmapHeight);
                ImGui::Text("Height Range: %.2f to %.2f", sHeightmapMin, sHeightmapMax);
                ImGui::Spacing();

                ImVec2 avail = ImGui::GetContentRegionAvail();
                float imageSize = std::min(avail.x, avail.y - 80);
                imageSize = std::max(imageSize, 128.0f);

                // Maintain aspect ratio
                float aspectRatio = static_cast<float>(sHeightmapWidth) / static_cast<float>(sHeightmapHeight);
                float displayW = imageSize;
                float displayH = imageSize;
                if (aspectRatio > 1.0f)
                    displayH = imageSize / aspectRatio;
                else
                    displayW = imageSize * aspectRatio;

                ImGui::Image((ImTextureID)(intptr_t)sHeightmapTexture, ImVec2(displayW, displayH));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Export as PNG...", ImVec2(150, 30)))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "HeightmapExportDialog",
                        "Save Heightmap PNG",
                        ".png",
                        config
                    );
                }

                ImGui::SameLine();
                if (ImGui::Button("Refresh", ImVec2(80, 30)))
                {
                    GenerateHeightmapTexture();
                }

                if (sHeightmapExportTimer > 0.0f)
                {
                    ImGui::Spacing();
                    bool success = sHeightmapExportMessage.find("Saved") != std::string::npos;
                    ImVec4 color = success ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    ImGui::TextColored(color, "%s", sHeightmapExportMessage.c_str());
                    sHeightmapExportTimer -= ImGui::GetIO().DeltaTime;
                }
            }
            else
            {
                ImGui::Text("No heightmap generated");
                if (ImGui::Button("Generate", ImVec2(100, 30)))
                {
                    GenerateHeightmapTexture();
                }
            }
        }
        ImGui::End();

        if (ImGuiFileDialog::Instance()->Display("HeightmapExportDialog", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string savePath = ImGuiFileDialog::Instance()->GetFilePathName();
                ExportHeightmapPNG(savePath);
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }

    void Reset()
    {
        sShowWindow = true;
        sExporting = false;
        sExportProgress = 0.0f;
        sLastExportMessage.clear();
        sShowExportResult = false;
        sExportComplete = false;
        sPendingExport = false;

        sShowHeightmapWindow = false;
        if (sHeightmapTexture != 0)
        {
            glDeleteTextures(1, &sHeightmapTexture);
            sHeightmapTexture = 0;
        }
        sHeightmapData.clear();
        sHeightmapWidth = 0;
        sHeightmapHeight = 0;
        sHeightmapExportMessage.clear();
        sHeightmapExportTimer = 0.0f;
    }

    void Draw(AppState& state)
    {
        DrawHeightmapWindow();

        if (gLoadedAreas.empty())
        {
            sWindowBottomY = 50.0f;
            return;
        }
        if (!sShowWindow)
        {
            sWindowBottomY = 50.0f;
            return;
        }

        if (ImGuiFileDialog::Instance()->Display("ExportFolderDialog", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                sPendingExportPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                sPendingExport = true;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (sPendingExport && !sExporting)
        {
            sPendingExport = false;
            sExporting = true;
            sExportProgress = 0.0f;
            sExportCurrentChunk = 0;
            sExportComplete = false;
            sShowExportResult = false;

            sExportSettings.outputPath = sPendingExportPath;

            int totalChunks = 0;
            for (const auto& area : gLoadedAreas)
            {
                if (area)
                {
                    for (const auto& chunk : area->getChunks())
                    {
                        if (chunk && chunk->isFullyInitialized())
                            totalChunks++;
                    }
                }
            }
            sExportTotalChunks = totalChunks;

            {
                std::lock_guard<std::mutex> lock(sExportMutex);
                sExportStatusText = "Starting export...";
            }

            std::thread([&state]() {
                ArchivePtr archive = nullptr;
                if (!state.archives.empty())
                    archive = state.archives[0];

                sExportResult = FBXExport::ExportAreasToFBX(
                    gLoadedAreas,
                    archive,
                    sExportSettings,
                    [](int current, int total, const std::string& status) {
                        sExportCurrentChunk = current;
                        sExportTotalChunks = total;
                        sExportProgress = (total > 0) ? static_cast<float>(current) / static_cast<float>(total) : 0.0f;

                        std::lock_guard<std::mutex> lock(sExportMutex);
                        sExportStatusText = status;
                    }
                );

                sExportProgress = 1.0f;
                sExportComplete = true;
            }).detach();
        }

        if (sExportComplete && sExporting)
        {
            sExporting = false;
            sExportComplete = false;
            sShowExportResult = true;
            sExportResultTimer = 8.0f;

            if (sExportResult.success)
            {
                sLastExportMessage = "Export successful!\n";
                sLastExportMessage += "File: " + sExportResult.outputFile + "\n";
                sLastExportMessage += "Vertices: " + std::to_string(sExportResult.vertexCount) + "\n";
                sLastExportMessage += "Triangles: " + std::to_string(sExportResult.triangleCount) + "\n";
                sLastExportMessage += "Chunks: " + std::to_string(sExportResult.chunkCount) + "\n";
                sLastExportMessage += "Textures: " + std::to_string(sExportResult.textureCount);
                sLastExportPath = sExportResult.outputFile;
            }
            else
            {
                sLastExportMessage = "Export failed: " + sExportResult.errorMessage;
                sLastExportPath.clear();
            }
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 10.0f, 50.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        if (ImGui::Begin("Area Info", &sShowWindow, flags))
        {
            AreaFilePtr area = gLoadedAreas.empty() ? nullptr : gLoadedAreas[0];

            if (area)
            {
                ImGui::Text("Loaded Areas: %zu", gLoadedAreas.size());
                ImGui::Separator();

                ImGui::Text("Tile: %d, %d", area->getTileX(), area->getTileY());

                glm::vec3 minB = area->getMinBounds();
                glm::vec3 maxB = area->getMaxBounds();
                glm::vec3 worldMin = area->getWorldMinBounds();
                glm::vec3 worldMax = area->getWorldMaxBounds();

                ImGui::Spacing();
                ImGui::Text("Local Bounds:");
                ImGui::Text("  Min: %.1f, %.1f, %.1f", minB.x, minB.y, minB.z);
                ImGui::Text("  Max: %.1f, %.1f, %.1f", maxB.x, maxB.y, maxB.z);

                ImGui::Spacing();
                ImGui::Text("World Bounds:");
                ImGui::Text("  Min: %.1f, %.1f, %.1f", worldMin.x, worldMin.y, worldMin.z);
                ImGui::Text("  Max: %.1f, %.1f, %.1f", worldMax.x, worldMax.y, worldMax.z);

                int validChunks = 0;
                for (const auto& c : area->getChunks())
                {
                    if (c && c->isFullyInitialized()) validChunks++;
                }

                ImGui::Spacing();
                ImGui::Text("Chunks: %d / 256", validChunks);
                ImGui::Text("Avg Height: %.2f", area->getAverageHeight());
                ImGui::Text("Max Height: %.2f", area->getMaxHeight());

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Display Heightmap", ImVec2(ImGui::GetContentRegionAvail().x, 26)))
                {
                    sShowHeightmapWindow = true;
                    GenerateHeightmapTexture();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Export Settings");
                ImGui::Spacing();

                if (sExporting)
                    ImGui::BeginDisabled();

                ImGui::Checkbox("Export Textures", &sExportSettings.exportTextures);
                ImGui::Checkbox("Export Normals", &sExportSettings.exportNormals);
                ImGui::Checkbox("Export UVs", &sExportSettings.exportUVs);

                ImGui::Spacing();
                ImGui::Text("Scale:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputFloat("##Scale", &sExportSettings.scale, 0.1f, 1.0f, "%.2f");
                if (sExportSettings.scale < 0.01f) sExportSettings.scale = 0.01f;
                if (sExportSettings.scale > 100.0f) sExportSettings.scale = 100.0f;

                if (sExporting)
                    ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (sExporting)
                {
                    std::string statusText;
                    {
                        std::lock_guard<std::mutex> lock(sExportMutex);
                        statusText = sExportStatusText;
                    }

                    ImGui::Text("%s", statusText.c_str());

                    float progress = sExportProgress.load();
                    int current = sExportCurrentChunk.load();
                    int total = sExportTotalChunks.load();

                    char progressText[64];
                    snprintf(progressText, sizeof(progressText), "%d / %d chunks", current, total);

                    ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 20), progressText);

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Exporting...");
                }
                else
                {
                    if (ImGui::Button("Export to FBX...", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
                    {
                        IGFD::FileDialogConfig config;
                        config.path = ".";
                        config.flags = ImGuiFileDialogFlags_Modal;
                        ImGuiFileDialog::Instance()->OpenDialog(
                            "ExportFolderDialog",
                            "Select Export Folder",
                            nullptr,
                            config
                        );
                    }
                }

                if (sShowExportResult && !sExporting)
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (sLastExportPath.empty())
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", sLastExportMessage.c_str());
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", sLastExportMessage.c_str());
                    }

                    sExportResultTimer -= ImGui::GetIO().DeltaTime;
                    if (sExportResultTimer <= 0.0f)
                    {
                        sShowExportResult = false;
                    }
                }
            }
            else
            {
                ImGui::Text("No area loaded");
            }

            sWindowBottomY = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
        }
        ImGui::End();
    }

    float GetWindowBottomY()
    {
        return sWindowBottomY;
    }
}