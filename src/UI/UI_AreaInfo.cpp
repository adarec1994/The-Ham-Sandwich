#include "UI_AreaInfo.h"
#include "UI_Globals.h"
#include "../export/FBXExport.h"
#include "../export/TerrainExport.h"
#include "../Area/AreaFile.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <glad/glad.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

#include <stb_image_write.h>

extern void SnapCameraToProp(AppState& state, const glm::vec3& position, float scale);

static std::string GetFilenameFromPath(const std::string& path)
{
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        return path.substr(lastSlash + 1);
    return path;
}

namespace UI_AreaInfo
{
    static bool sShowWindow = true;
    static std::string sLastExportPath;
    static std::string sLastExportMessage;
    static bool sShowExportResult = false;
    static float sExportResultTimer = 0.0f;

    static FBXExport::ExportSettings sExportSettings;
    static TerrainExport::ExportSettings sTerrainSettings;

    static std::atomic<bool> sExporting{false};
    static std::atomic<float> sExportProgress{0.0f};
    static std::atomic<int> sExportCurrentChunk{0};
    static std::atomic<int> sExportTotalChunks{0};
    static std::string sExportStatusText;
    static std::mutex sExportMutex;
    static FBXExport::ExportResult sExportResult;
    static TerrainExport::ExportResult sTerrainResult;
    static bool sExportComplete = false;

    static bool sPendingExport = false;
    static bool sPendingTerrainExport = false;
    static std::string sPendingExportPath;
    static ArchivePtr sPendingArchive;

    static float sWindowBottomY = 50.0f;

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
            const auto& chunks = area->getChunks();
            bool hasVerts = false;
            for (const auto& chunk : chunks)
            {
                if (chunk && !chunk->getVertices().empty())
                {
                    hasVerts = true;
                    break;
                }
            }
            if (hasVerts)
            {
                minTileX = std::min(minTileX, area->getTileX());
                maxTileX = std::max(maxTileX, area->getTileX());
                minTileY = std::min(minTileY, area->getTileY());
                maxTileY = std::max(maxTileY, area->getTileY());
            }
        }

        if (minTileX > maxTileX || minTileY > maxTileY)
        {
            sHeightmapData.clear();
            sHeightmapWidth = 0;
            sHeightmapHeight = 0;
            return;
        }

        int tilesWide = maxTileX - minTileX + 1;
        int tilesHigh = maxTileY - minTileY + 1;
        int totalWidth = tilesWide * AREA_SIZE;
        int totalHeight = tilesHigh * AREA_SIZE;

        constexpr int MAX_DIM = 4096;
        float scale = 1.0f;
        if (totalWidth > MAX_DIM || totalHeight > MAX_DIM)
        {
            scale = static_cast<float>(MAX_DIM) / std::max(totalWidth, totalHeight);
            totalWidth = static_cast<int>(totalWidth * scale);
            totalHeight = static_cast<int>(totalHeight * scale);
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
            for (size_t chunkIdx = 0; chunkIdx < chunks.size(); chunkIdx++)
            {
                const auto& chunk = chunks[chunkIdx];
                if (!chunk) continue;
                const auto& verts = chunk->getVertices();
                if (verts.empty()) continue;

                int chunkX = chunkIdx % 16;
                int chunkY = chunkIdx / 16;

                for (int vy = 0; vy < 17; vy++)
                {
                    for (int vx = 0; vx < 17; vx++)
                    {
                        int vertIdx = vy * 17 + vx;
                        if (vertIdx >= static_cast<int>(verts.size())) continue;
                        float h = verts[vertIdx].y;

                        int pixelX = chunkY * 16 + vx;
                        int pixelY = chunkX * 16 + vy;
                        if (pixelX >= AREA_SIZE) pixelX = AREA_SIZE - 1;
                        if (pixelY >= AREA_SIZE) pixelY = AREA_SIZE - 1;

                        int imgX = static_cast<int>((areaTileX * AREA_SIZE + pixelX) * scale);
                        int imgY = static_cast<int>((areaTileY * AREA_SIZE + pixelY) * scale);
                        imgY = totalHeight - 1 - imgY;

                        if (imgX >= 0 && imgX < totalWidth && imgY >= 0 && imgY < totalHeight)
                        {
                            int idx = imgY * totalWidth + imgX;
                            heights[idx] = h;
                            minHeight = std::min(minHeight, h);
                            maxHeight = std::max(maxHeight, h);
                        }
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
            glGenTextures(1, &sHeightmapTexture);

        glBindTexture(GL_TEXTURE_2D, sHeightmapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sHeightmapWidth, sHeightmapHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, sHeightmapData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static std::string GetHeightmapFilename()
    {
        if (gLoadedAreas.empty() || !gLoadedAreas[0]) return "heightmap.png";
        std::wstring wpath = gLoadedAreas[0]->getPath();
        std::string path;
        for (wchar_t c : wpath) path += (c < 128) ? static_cast<char>(c) : '_';
        size_t lastSlash = path.rfind('/');
        if (lastSlash == std::string::npos) lastSlash = path.rfind('\\');
        std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        size_t ext = filename.rfind('.');
        if (ext != std::string::npos) filename = filename.substr(0, ext);
        if (filename.size() > 5)
        {
            size_t dot = filename.rfind('.');
            if (dot != std::string::npos) filename = filename.substr(0, dot);
        }
        return filename + "_heightmap.png";
    }

    static void DrawHeightmapWindow()
    {
        if (!sShowHeightmapWindow) return;

        ImGui::SetNextWindowSize(ImVec2(500, 550), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Heightmap Preview", &sShowHeightmapWindow))
        {
            if (sHeightmapTexture != 0 && sHeightmapWidth > 0 && sHeightmapHeight > 0)
            {
                ImGui::Text("Size: %d x %d", sHeightmapWidth, sHeightmapHeight);
                ImGui::Text("Height Range: %.2f to %.2f", sHeightmapMin, sHeightmapMax);
                ImGui::Separator();

                ImVec2 avail = ImGui::GetContentRegionAvail();
                avail.y -= 40;

                float imgAspect = static_cast<float>(sHeightmapWidth) / static_cast<float>(sHeightmapHeight);
                float availAspect = avail.x / avail.y;

                ImVec2 imgSize;
                if (imgAspect > availAspect)
                {
                    imgSize.x = avail.x;
                    imgSize.y = avail.x / imgAspect;
                }
                else
                {
                    imgSize.y = avail.y;
                    imgSize.x = avail.y * imgAspect;
                }

                ImGui::Image((ImTextureID)(intptr_t)sHeightmapTexture, imgSize);

                ImGui::Separator();

                if (ImGui::Button("Save Heightmap...", ImVec2(150, 0)))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = GetHeightmapFilename();
                    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
                    ImGuiFileDialog::Instance()->OpenDialog("SaveHeightmapDialog", "Save Heightmap As", ".png", config);
                }

                if (sHeightmapExportTimer > 0.0f)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", sHeightmapExportMessage.c_str());
                    sHeightmapExportTimer -= ImGui::GetIO().DeltaTime;
                }
            }
            else
            {
                ImGui::Text("No heightmap data available");
            }
        }
        ImGui::End();

        if (ImGuiFileDialog::Instance()->Display("SaveHeightmapDialog", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filepath = ImGuiFileDialog::Instance()->GetFilePathName();
                if (!sHeightmapData.empty() && sHeightmapWidth > 0 && sHeightmapHeight > 0)
                {
                    if (stbi_write_png(filepath.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapData.data(), sHeightmapWidth * 4))
                    {
                        sHeightmapExportMessage = "Saved!";
                        sHeightmapExportTimer = 3.0f;
                    }
                    else
                    {
                        sHeightmapExportMessage = "Failed to save!";
                        sHeightmapExportTimer = 3.0f;
                    }
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }

    void Reset()
    {
        sShowWindow = true;
        sLastExportPath.clear();
        sLastExportMessage.clear();
        sShowExportResult = false;
        sExportResultTimer = 0.0f;
        sExporting = false;
        sExportProgress = 0.0f;
        sExportCurrentChunk = 0;
        sExportTotalChunks = 0;
        sExportStatusText.clear();
        sExportComplete = false;
        sPendingExport = false;
        sPendingTerrainExport = false;
        sPendingExportPath.clear();
        sShowHeightmapWindow = false;
        sWindowBottomY = 50.0f;
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

        if (ImGuiFileDialog::Instance()->Display("ExportTerrainDialog", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                sPendingExportPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                sPendingTerrainExport = true;
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
                if (area) for (const auto& chunk : area->getChunks())
                    if (chunk && chunk->isFullyInitialized()) totalChunks++;
            sExportTotalChunks = totalChunks;

            {
                std::lock_guard<std::mutex> lock(sExportMutex);
                sExportStatusText = "Starting FBX export...";
            }

            std::thread([&state]() {
                ArchivePtr archive = nullptr;
                if (!state.archives.empty()) archive = state.archives[0];

                sExportResult = FBXExport::ExportAreasToFBX(
                    gLoadedAreas, archive, sExportSettings,
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

        if (sPendingTerrainExport && !sExporting)
        {
            sPendingTerrainExport = false;
            sExporting = true;
            sExportProgress = 0.0f;
            sExportCurrentChunk = 0;
            sExportComplete = false;
            sShowExportResult = false;

            sTerrainSettings.outputPath = sPendingExportPath;
            sTerrainSettings.scale = sExportSettings.scale;

            int totalChunks = 0;
            for (const auto& area : gLoadedAreas)
                if (area) for (const auto& chunk : area->getChunks())
                    if (chunk && chunk->isFullyInitialized()) totalChunks++;
            sExportTotalChunks = totalChunks;

            {
                std::lock_guard<std::mutex> lock(sExportMutex);
                sExportStatusText = "Starting terrain export...";
            }

            std::thread([]() {
                sTerrainResult = TerrainExport::ExportAreasToTerrain(
                    gLoadedAreas, sTerrainSettings,
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

            bool useTerrain = !sTerrainResult.outputFile.empty();

            if (useTerrain)
            {
                if (sTerrainResult.success)
                {
                    sLastExportMessage = "Terrain Export successful!\n";
                    sLastExportMessage += "File: " + sTerrainResult.outputFile + "\n";
                    sLastExportMessage += "Chunks: " + std::to_string(sTerrainResult.chunkCount) + "\n";
                    sLastExportMessage += "Layers: " + std::to_string(sTerrainResult.textureCount);
                    sLastExportPath = sTerrainResult.outputFile;
                }
                else
                {
                    sLastExportMessage = "Terrain Export failed: " + sTerrainResult.errorMessage;
                    sLastExportPath.clear();
                }
                sTerrainResult = TerrainExport::ExportResult();
            }
            else
            {
                if (sExportResult.success)
                {
                    sLastExportMessage = "FBX Export successful!\n";
                    sLastExportMessage += "File: " + sExportResult.outputFile + "\n";
                    sLastExportMessage += "Vertices: " + std::to_string(sExportResult.vertexCount) + "\n";
                    sLastExportMessage += "Triangles: " + std::to_string(sExportResult.triangleCount) + "\n";
                    sLastExportMessage += "Chunks: " + std::to_string(sExportResult.chunkCount) + "\n";
                    sLastExportMessage += "Textures: " + std::to_string(sExportResult.textureCount);
                    sLastExportPath = sExportResult.outputFile;
                }
                else
                {
                    sLastExportMessage = "FBX Export failed: " + sExportResult.errorMessage;
                    sLastExportPath.clear();
                }
                sExportResult = FBXExport::ExportResult();
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
                    if (c && c->isFullyInitialized()) validChunks++;

                ImGui::Spacing();
                ImGui::Text("Chunks: %d / 256", validChunks);
                ImGui::Text("Avg Height: %.2f", area->getAverageHeight());
                ImGui::Text("Max Height: %.2f", area->getMaxHeight());

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Props");
                ImGui::Separator();

                size_t totalProps = 0;
                size_t loadedProps = 0;
                for (const auto& a : gLoadedAreas)
                {
                    if (!a) continue;
                    totalProps += a->getPropCount();
                    loadedProps += a->getLoadedPropCount();
                }

                ImGui::Text("Total: %zu", totalProps);
                ImGui::Text("Loaded (with render): %zu", loadedProps);

                if (totalProps > 0 && ImGui::TreeNode("Prop Details"))
                {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Double-click to focus camera");
                    ImGui::Spacing();

                    int displayCount = 0;
                    const int maxDisplay = 100;
                    int globalIndex = 0;
                    static int sSelectedPropIndex = -1;

                    for (const auto& a : gLoadedAreas)
                    {
                        if (!a) continue;
                        const auto& props = a->getProps();
                        glm::vec3 areaOffset = a->getWorldOffset();

                        for (size_t i = 0; i < props.size() && displayCount < maxDisplay; i++)
                        {
                            const auto& prop = props[i];
                            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                            if (prop.loaded && prop.render)
                                color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                            else if (prop.loaded && !prop.render)
                                color = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
                            else
                                color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

                            bool isSelected = (sSelectedPropIndex == globalIndex);
                            if (isSelected)
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                            else
                                ImGui::PushStyleColor(ImGuiCol_Text, color);

                            std::string filename = GetFilenameFromPath(prop.path);
                            if (filename.empty()) filename = "(no path)";

                            char label[256];
                            snprintf(label, sizeof(label), "[%u] %s##prop%d", prop.uniqueID, filename.c_str(), globalIndex);

                            if (ImGui::Selectable(label, isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                            {
                                sSelectedPropIndex = globalIndex;

                                if (ImGui::IsMouseDoubleClicked(0))
                                {
                                    // Props are in world coords, camera uses rendering coords
                                    // areaOffset already accounts for this in the UI display
                                    glm::vec3 worldPos = prop.position + areaOffset;
                                    SnapCameraToProp(state, worldPos, prop.scale);
                                }
                            }

                            ImGui::PopStyleColor();

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("Full Path: %s", prop.path.c_str());
                                ImGui::Separator();
                                glm::vec3 worldPos = prop.position + areaOffset;
                                ImGui::Text("Local Pos: %.1f, %.1f, %.1f", prop.position.x, prop.position.y, prop.position.z);
                                ImGui::Text("World Pos: %.1f, %.1f, %.1f", worldPos.x, worldPos.y, worldPos.z);
                                ImGui::Text("Scale: %.3f", prop.scale);
                                ImGui::Text("ModelType: %d", static_cast<int>(prop.modelType));
                                ImGui::Text("Loaded: %s, Has Render: %s", prop.loaded ? "Yes" : "No", prop.render ? "Yes" : "No");
                                ImGui::EndTooltip();
                            }
                            displayCount++;
                            globalIndex++;
                        }
                    }
                    if (totalProps > maxDisplay)
                        ImGui::Text("... and %zu more", totalProps - maxDisplay);
                    ImGui::TreePop();
                }

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

                if (sExporting) ImGui::BeginDisabled();

                ImGui::Checkbox("Export Textures", &sExportSettings.exportTextures);

                ImGui::Spacing();
                ImGui::Text("Scale:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputFloat("##Scale", &sExportSettings.scale, 0.1f, 1.0f, "%.2f");
                if (sExportSettings.scale < 0.01f) sExportSettings.scale = 0.01f;
                if (sExportSettings.scale > 100.0f) sExportSettings.scale = 100.0f;

                if (sExporting) ImGui::EndDisabled();

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
                    snprintf(progressText, sizeof(progressText), "%d / %d", current, total);

                    ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 20), progressText);
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Exporting...");
                }
                else
                {
                    if (ImGui::Button("Export for Blender (.wsterrain)", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
                    {
                        IGFD::FileDialogConfig config;
                        config.path = ".";
                        config.flags = ImGuiFileDialogFlags_Modal;
                        ImGuiFileDialog::Instance()->OpenDialog("ExportTerrainDialog", "Select Export Folder", nullptr, config);
                    }

                    ImGui::Spacing();

                    if (ImGui::Button("Export to FBX (baked)", ImVec2(ImGui::GetContentRegionAvail().x, 24)))
                    {
                        IGFD::FileDialogConfig config;
                        config.path = ".";
                        config.flags = ImGuiFileDialogFlags_Modal;
                        ImGuiFileDialog::Instance()->OpenDialog("ExportFolderDialog", "Select Export Folder", nullptr, config);
                    }
                }

                if (sShowExportResult && !sExporting)
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (sLastExportPath.empty())
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", sLastExportMessage.c_str());
                    else
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", sLastExportMessage.c_str());

                    sExportResultTimer -= ImGui::GetIO().DeltaTime;
                    if (sExportResultTimer <= 0.0f)
                        sShowExportResult = false;
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