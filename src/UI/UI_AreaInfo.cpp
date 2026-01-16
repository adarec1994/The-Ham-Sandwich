#include "UI_AreaInfo.h"
#include "UI_Globals.h"
#include "../export/FBXExport.h"
#include "../Area/AreaFile.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <filesystem>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>

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

    void Reset()
    {
        sShowWindow = true;
        sExporting = false;
        sExportProgress = 0.0f;
        sLastExportMessage.clear();
        sShowExportResult = false;
        sExportComplete = false;
        sPendingExport = false;
    }

    void Draw(AppState& state)
    {
        if (gLoadedAreas.empty()) return;
        if (!sShowWindow) return;

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

        ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 320.0f, 10.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;

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
        }
        ImGui::End();
    }
}