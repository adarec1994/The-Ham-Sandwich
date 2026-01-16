#include "UI_AreaInfo.h"
#include "UI_Globals.h"
#include "../export/FBXExport.h"
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
        {
            glGenTextures(1, &sHeightmapTexture);
        }

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
        if (gLoadedAreas.empty()) return "heightmap";

        const auto& path = gLoadedAreas[0]->getPath();
        if (path.empty()) return "heightmap";

        size_t lastSlash = path.find_last_of(L"/\\");
        std::wstring filename = (lastSlash != std::wstring::npos) ? path.substr(lastSlash + 1) : path;

        size_t ext = filename.rfind(L".area");
        if (ext != std::wstring::npos)
            filename = filename.substr(0, ext);

        if (gLoadedAreas.size() > 1)
        {
            size_t dot = filename.rfind(L'.');
            if (dot != std::wstring::npos && dot >= 2)
            {
                bool isHex = true;
                for (size_t i = dot + 1; i < filename.size() && isHex; i++)
                {
                    wchar_t c = filename[i];
                    if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')))
                        isHex = false;
                }
                if (isHex)
                    filename = filename.substr(0, dot);
            }
        }

        std::string result;
        for (wchar_t c : filename)
            result += (c < 128) ? static_cast<char>(c) : '_';

        return result.empty() ? "heightmap" : result;
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
            fullPath += ".png";

        if (stbi_write_png(fullPath.c_str(), sHeightmapWidth, sHeightmapHeight, 4, sHeightmapData.data(), sHeightmapWidth * 4))
            sHeightmapExportMessage = "Saved: " + fullPath;
        else
            sHeightmapExportMessage = "Failed to save heightmap";
        sHeightmapExportTimer = 4.0f;
    }

    static void ExportHeightmapJPEG(const std::string& path)
    {
        if (sHeightmapData.empty() || sHeightmapWidth == 0 || sHeightmapHeight == 0)
        {
            sHeightmapExportMessage = "No heightmap data to export";
            sHeightmapExportTimer = 3.0f;
            return;
        }

        std::string fullPath = path;
        if (fullPath.find(".jpg") == std::string::npos && fullPath.find(".JPG") == std::string::npos &&
            fullPath.find(".jpeg") == std::string::npos && fullPath.find(".JPEG") == std::string::npos)
            fullPath += ".jpg";

        std::vector<uint8_t> rgb(sHeightmapWidth * sHeightmapHeight * 3);
        for (int i = 0; i < sHeightmapWidth * sHeightmapHeight; i++)
        {
            rgb[i * 3 + 0] = sHeightmapData[i * 4 + 0];
            rgb[i * 3 + 1] = sHeightmapData[i * 4 + 1];
            rgb[i * 3 + 2] = sHeightmapData[i * 4 + 2];
        }

        if (stbi_write_jpg(fullPath.c_str(), sHeightmapWidth, sHeightmapHeight, 3, rgb.data(), 95))
            sHeightmapExportMessage = "Saved: " + fullPath;
        else
            sHeightmapExportMessage = "Failed to save heightmap";
        sHeightmapExportTimer = 4.0f;
    }

    static void ExportHeightmapTIFF(const std::string& path)
    {
        if (sHeightmapData.empty() || sHeightmapWidth == 0 || sHeightmapHeight == 0)
        {
            sHeightmapExportMessage = "No heightmap data to export";
            sHeightmapExportTimer = 3.0f;
            return;
        }

        std::string fullPath = path;
        if (fullPath.find(".tif") == std::string::npos && fullPath.find(".TIF") == std::string::npos)
            fullPath += ".tif";

        std::vector<uint16_t> heightData16(sHeightmapWidth * sHeightmapHeight);
        for (int i = 0; i < sHeightmapWidth * sHeightmapHeight; i++)
            heightData16[i] = static_cast<uint16_t>(sHeightmapData[i * 4] * 257);

        std::ofstream out(fullPath, std::ios::binary);
        if (!out)
        {
            sHeightmapExportMessage = "Failed to create file";
            sHeightmapExportTimer = 3.0f;
            return;
        }

        uint16_t byteOrder = 0x4949;
        uint16_t magic = 42;
        uint32_t ifdOffset = 8;
        out.write(reinterpret_cast<char*>(&byteOrder), 2);
        out.write(reinterpret_cast<char*>(&magic), 2);
        out.write(reinterpret_cast<char*>(&ifdOffset), 4);

        uint16_t numEntries = 8;
        out.write(reinterpret_cast<char*>(&numEntries), 2);

        auto writeTag = [&](uint16_t tag, uint16_t type, uint32_t count, uint32_t value) {
            out.write(reinterpret_cast<char*>(&tag), 2);
            out.write(reinterpret_cast<char*>(&type), 2);
            out.write(reinterpret_cast<char*>(&count), 4);
            out.write(reinterpret_cast<char*>(&value), 4);
        };

        uint32_t stripOffset = 8 + 2 + (12 * numEntries) + 4;
        uint32_t stripBytes = sHeightmapWidth * sHeightmapHeight * 2;

        writeTag(256, 3, 1, sHeightmapWidth);
        writeTag(257, 3, 1, sHeightmapHeight);
        writeTag(258, 3, 1, 16);
        writeTag(259, 3, 1, 1);
        writeTag(262, 3, 1, 1);
        writeTag(273, 4, 1, stripOffset);
        writeTag(278, 3, 1, sHeightmapHeight);
        writeTag(279, 4, 1, stripBytes);

        uint32_t nextIFD = 0;
        out.write(reinterpret_cast<char*>(&nextIFD), 4);

        out.write(reinterpret_cast<char*>(heightData16.data()), stripBytes);
        out.close();

        sHeightmapExportMessage = "Saved: " + fullPath;
        sHeightmapExportTimer = 4.0f;
    }

    static void ExportHeightmapDDS(const std::string& path)
    {
        if (sHeightmapData.empty() || sHeightmapWidth == 0 || sHeightmapHeight == 0)
        {
            sHeightmapExportMessage = "No heightmap data to export";
            sHeightmapExportTimer = 3.0f;
            return;
        }

        std::string fullPath = path;
        if (fullPath.find(".dds") == std::string::npos && fullPath.find(".DDS") == std::string::npos)
            fullPath += ".dds";

        std::vector<uint16_t> heightData16(sHeightmapWidth * sHeightmapHeight);
        for (int i = 0; i < sHeightmapWidth * sHeightmapHeight; i++)
            heightData16[i] = static_cast<uint16_t>(sHeightmapData[i * 4] * 257);

        std::ofstream out(fullPath, std::ios::binary);
        if (!out)
        {
            sHeightmapExportMessage = "Failed to create file";
            sHeightmapExportTimer = 3.0f;
            return;
        }

        uint32_t magic = 0x20534444;
        out.write(reinterpret_cast<char*>(&magic), 4);

        uint8_t header[124] = {0};
        *reinterpret_cast<uint32_t*>(&header[0]) = 124;
        *reinterpret_cast<uint32_t*>(&header[4]) = 0x1 | 0x2 | 0x4 | 0x1000;
        *reinterpret_cast<uint32_t*>(&header[8]) = sHeightmapHeight;
        *reinterpret_cast<uint32_t*>(&header[12]) = sHeightmapWidth;
        *reinterpret_cast<uint32_t*>(&header[16]) = sHeightmapWidth * 2;

        uint8_t* pf = &header[72];
        *reinterpret_cast<uint32_t*>(&pf[0]) = 32;
        *reinterpret_cast<uint32_t*>(&pf[4]) = 0x20000;
        *reinterpret_cast<uint32_t*>(&pf[8]) = 0;
        *reinterpret_cast<uint32_t*>(&pf[12]) = 16;
        *reinterpret_cast<uint32_t*>(&pf[16]) = 0xFFFF;
        *reinterpret_cast<uint32_t*>(&pf[20]) = 0;
        *reinterpret_cast<uint32_t*>(&pf[24]) = 0;
        *reinterpret_cast<uint32_t*>(&pf[28]) = 0;

        *reinterpret_cast<uint32_t*>(&header[104]) = 0x1000;

        out.write(reinterpret_cast<char*>(header), 124);
        out.write(reinterpret_cast<char*>(heightData16.data()), sHeightmapWidth * sHeightmapHeight * 2);
        out.close();

        sHeightmapExportMessage = "Saved: " + fullPath;
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
                ImGui::Spacing();

                ImVec2 avail = ImGui::GetContentRegionAvail();
                float imageSize = std::min(avail.x, avail.y - 80);
                imageSize = std::max(imageSize, 128.0f);

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

                std::string suggestedName = GetHeightmapFilename();

                if (ImGui::Button("PNG", ImVec2(60, 30)))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = suggestedName + ".png";
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::Instance()->OpenDialog("HeightmapExportPNG", "Save Heightmap PNG", ".png", config);
                }
                ImGui::SameLine();
                if (ImGui::Button("JPEG", ImVec2(60, 30)))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = suggestedName + ".jpg";
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::Instance()->OpenDialog("HeightmapExportJPEG", "Save Heightmap JPEG", ".jpg", config);
                }
                ImGui::SameLine();
                if (ImGui::Button("TIFF", ImVec2(60, 30)))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = suggestedName + ".tif";
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::Instance()->OpenDialog("HeightmapExportTIFF", "Save Heightmap TIFF", ".tif", config);
                }
                ImGui::SameLine();
                if (ImGui::Button("DDS", ImVec2(60, 30)))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    config.fileName = suggestedName + ".dds";
                    config.flags = ImGuiFileDialogFlags_Modal;
                    ImGuiFileDialog::Instance()->OpenDialog("HeightmapExportDDS", "Save Heightmap DDS", ".dds", config);
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

        if (ImGuiFileDialog::Instance()->Display("HeightmapExportPNG", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
                ExportHeightmapPNG(ImGuiFileDialog::Instance()->GetFilePathName());
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("HeightmapExportJPEG", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
                ExportHeightmapJPEG(ImGuiFileDialog::Instance()->GetFilePathName());
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("HeightmapExportTIFF", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
                ExportHeightmapTIFF(ImGuiFileDialog::Instance()->GetFilePathName());
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("HeightmapExportDDS", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
                ExportHeightmapDDS(ImGuiFileDialog::Instance()->GetFilePathName());
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