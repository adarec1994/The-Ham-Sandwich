#include "UI_ChunkTextures.h"
#include "UI_Globals.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <imgui.h>
#include <glad/glad.h>
#include <vector>

namespace UI_ChunkTextures
{
    struct ChunkTexturePreview
    {
        GLuint textureID = 0;
        int width = 0;
        int height = 0;
        std::string name;
        bool showUV = false;
        bool ownsTexture = false;
    };

    static std::vector<ChunkTexturePreview> sPreviews;
    static AreaChunkRenderPtr sLastChunk = nullptr;
    static GLuint sUVTexture = 0;
    static bool sWindowOpen = true;

    static GLuint CreateTexture(const std::vector<uint8_t>& data, int w, int h)
    {
        if (data.empty() || w <= 0 || h <= 0) return 0;

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    static GLuint CreateUVTexture(int w, int h)
    {
        std::vector<uint8_t> data(w * h * 4);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                int idx = (y * w + x) * 4;
                float u = static_cast<float>(x) / (w - 1);
                float v = static_cast<float>(y) / (h - 1);

                data[idx + 0] = static_cast<uint8_t>(u * 255);
                data[idx + 1] = static_cast<uint8_t>(v * 255);
                data[idx + 2] = 0;
                data[idx + 3] = 255;

                if ((x % 16 == 0) || (y % 16 == 0))
                {
                    data[idx + 0] = 255;
                    data[idx + 1] = 255;
                    data[idx + 2] = 255;
                }
            }
        }
        return CreateTexture(data, w, h);
    }

    static void ClearPreviews()
    {
        for (auto& p : sPreviews)
        {
            if (p.ownsTexture && p.textureID != 0)
                glDeleteTextures(1, &p.textureID);
        }
        sPreviews.clear();

        if (sUVTexture != 0)
        {
            glDeleteTextures(1, &sUVTexture);
            sUVTexture = 0;
        }
    }

    static void LoadChunkTextures(const AreaChunkRenderPtr& chunk, const ArchivePtr& archive)
    {
        if (!chunk) return;
        ClearPreviews();

        const auto& blendMap = chunk->getBlendMap();
        if (!blendMap.empty())
        {
            ChunkTexturePreview p;
            p.width = 65;
            p.height = 65;
            p.name = "Blend Map";
            p.textureID = CreateTexture(blendMap, 65, 65);
            p.ownsTexture = true;
            sPreviews.push_back(p);
        }

        const auto& colorMap = chunk->getColorMap();
        if (!colorMap.empty())
        {
            ChunkTexturePreview p;
            p.width = 65;
            p.height = 65;
            p.name = "Color Map";
            p.textureID = CreateTexture(colorMap, 65, 65);
            p.ownsTexture = true;
            sPreviews.push_back(p);
        }

        const uint32_t* layerIds = chunk->getWorldLayerIDs();
        auto& texMgr = TerrainTexture::Manager::Instance();

        for (int i = 0; i < 4; i++)
        {
            uint32_t layerId = layerIds[i];
            if (layerId == 0) continue;

            const auto* cached = texMgr.GetLayerTexture(archive, layerId);
            if (!cached || !cached->loaded) continue;

            if (cached->diffuse != 0)
            {
                ChunkTexturePreview p;
                p.textureID = cached->diffuse;
                p.width = cached->width;
                p.height = cached->height;
                p.name = "Layer " + std::to_string(i) + " Diffuse (ID:" + std::to_string(layerId) + ")";
                p.ownsTexture = false;
                sPreviews.push_back(p);
            }

            if (cached->normal != 0)
            {
                ChunkTexturePreview p;
                p.textureID = cached->normal;
                p.width = cached->width;
                p.height = cached->height;
                p.name = "Layer " + std::to_string(i) + " Normal (ID:" + std::to_string(layerId) + ")";
                p.ownsTexture = false;
                sPreviews.push_back(p);
            }
        }

        sUVTexture = CreateUVTexture(256, 256);
    }

    void Reset()
    {
        ClearPreviews();
        sLastChunk = nullptr;
        sWindowOpen = true;
    }

    void Draw(AppState& state)
    {
        if (!gSelectedChunk)
        {
            if (sLastChunk)
            {
                ClearPreviews();
                sLastChunk = nullptr;
            }
            return;
        }

        if (!sWindowOpen) return;

        if (gSelectedChunk != sLastChunk)
        {
            ArchivePtr archive = nullptr;
            if (!state.archives.empty())
                archive = state.archives[0];
            LoadChunkTextures(gSelectedChunk, archive);
            sLastChunk = gSelectedChunk;
        }
        
        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        
        std::string title = "Chunk " + std::to_string(gSelectedChunkIndex) + "###ChunkTex";
        
        if (ImGui::Begin(title.c_str(), &sWindowOpen))
        {
            ImGui::Text("Chunk Index: %d", gSelectedChunkIndex);
            ImGui::Text("Area: %s", gSelectedChunkAreaName.c_str());
            
            glm::vec3 minB = gSelectedChunk->getMinBounds();
            glm::vec3 maxB = gSelectedChunk->getMaxBounds();
            ImGui::Text("Min: (%.1f, %.1f, %.1f)", minB.x, minB.y, minB.z);
            ImGui::Text("Max: (%.1f, %.1f, %.1f)", maxB.x, maxB.y, maxB.z);
            
            const uint32_t* layers = gSelectedChunk->getWorldLayerIDs();
            ImGui::Text("Layers: %u, %u, %u, %u", layers[0], layers[1], layers[2], layers[3]);
            
            ImGui::Separator();
            
            if (sPreviews.empty())
            {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No textures available");
            }
            else
            {
                float previewSize = 128.0f;
                
                for (size_t i = 0; i < sPreviews.size(); i++)
                {
                    auto& p = sPreviews[i];
                    if (p.textureID == 0) continue;
                    
                    ImGui::PushID(static_cast<int>(i));
                    
                    if (ImGui::CollapsingHeader(p.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Text("Size: %dx%d", p.width, p.height);
                        
                        ImGui::Checkbox("Show UV", &p.showUV);
                        
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImGui::Image((ImTextureID)(intptr_t)p.textureID, ImVec2(previewSize, previewSize));
                        
                        if (p.showUV && sUVTexture != 0)
                        {
                            ImGui::GetWindowDrawList()->AddImage(
                                (ImTextureID)(intptr_t)sUVTexture,
                                pos,
                                ImVec2(pos.x + previewSize, pos.y + previewSize),
                                ImVec2(0, 0), ImVec2(1, 1),
                                IM_COL32(255, 255, 255, 128)
                            );
                        }
                        
                        if (ImGui::IsItemHovered())
                        {
                            ImVec2 mouse = ImGui::GetMousePos();
                            float u = (mouse.x - pos.x) / previewSize;
                            float v = (mouse.y - pos.y) / previewSize;
                            if (u >= 0 && u <= 1 && v >= 0 && v <= 1)
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("UV: (%.3f, %.3f)", u, v);
                                ImGui::Text("Pixel: (%d, %d)", (int)(u * p.width), (int)(v * p.height));
                                ImGui::EndTooltip();
                            }
                        }
                    }
                    
                    ImGui::PopID();
                }
            }
            
            ImGui::Separator();
            
            if (ImGui::Button("Deselect", ImVec2(-1, 0)))
            {
                gSelectedChunk = nullptr;
                gSelectedChunkIndex = -1;
            }
        }
        ImGui::End();
    }
}