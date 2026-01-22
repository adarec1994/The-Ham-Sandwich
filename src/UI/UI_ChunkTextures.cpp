#include "UI_ChunkTextures.h"
#include "UI_Globals.h"
#include "UI_AreaInfo.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainTexture.h"
#include "../Archive.h"
#include "../tex/tex.h"
#include <imgui.h>
#include <d3d11.h>
#include <vector>
#include <algorithm>

extern ID3D11Device* gDevice;
extern ID3D11DeviceContext* gContext;

namespace UI_ChunkTextures
{
    struct ChunkTexturePreview
    {
        ID3D11ShaderResourceView* textureID = nullptr;
        int width = 0;
        int height = 0;
        std::string name;
        bool ownsTexture = false;
    };

    static std::vector<ChunkTexturePreview> sPreviews;
    static AreaChunkRenderPtr sLastChunk = nullptr;
    static bool sWindowOpen = true;

    static ID3D11ShaderResourceView* CreateTexture(const std::vector<uint8_t>& data, int w, int h)
    {
        if (data.empty() || w <= 0 || h <= 0 || !gDevice) return nullptr;

        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = w;
        texDesc.Height = h;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_IMMUTABLE;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data.data();
        initData.SysMemPitch = w * 4;

        ID3D11Texture2D* texture = nullptr;
        HRESULT hr = gDevice->CreateTexture2D(&texDesc, &initData, &texture);
        if (FAILED(hr)) return nullptr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = gDevice->CreateShaderResourceView(texture, &srvDesc, &srv);
        texture->Release();

        if (FAILED(hr)) return nullptr;
        return srv;
    }

    static void ClearPreviews()
    {
        for (auto& p : sPreviews)
        {
            if (p.ownsTexture && p.textureID != nullptr)
                p.textureID->Release();
        }
        sPreviews.clear();
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

            if (cached->diffuse != nullptr)
            {
                ChunkTexturePreview p;
                p.textureID = cached->diffuse.Get();
                p.width = cached->width;
                p.height = cached->height;
                p.name = "Layer " + std::to_string(i) + " Diffuse (ID:" + std::to_string(layerId) + ")";
                p.ownsTexture = false;
                sPreviews.push_back(p);
            }

            if (cached->normal != nullptr)
            {
                ChunkTexturePreview p;
                p.textureID = cached->normal.Get();
                p.width = cached->width;
                p.height = cached->height;
                p.name = "Layer " + std::to_string(i) + " Normal (ID:" + std::to_string(layerId) + ")";
                p.ownsTexture = false;
                sPreviews.push_back(p);
            }
        }
    }

    void Reset()
    {
        ClearPreviews();
        sLastChunk = nullptr;
        sWindowOpen = true;
    }

    static ID3D11ShaderResourceView* sPreviewTexture = nullptr;
    static int sPreviewWidth = 0;
    static int sPreviewHeight = 0;
    static std::string sPreviewName;
    static bool sShowPreview = false;

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

        std::string title = "Chunk " + std::to_string(gSelectedChunkIndex) + "###ChunkTex";

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float topY = UI_AreaInfo::GetWindowBottomY() + 10.0f;
        float maxHeight = viewport->Size.y - topY - 20.0f;

        ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 10.0f, topY), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 100), ImVec2(300, maxHeight));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove;

        if (ImGui::Begin(title.c_str(), &sWindowOpen, flags))
        {
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
                    if (p.textureID == nullptr) continue;

                    ImGui::PushID(static_cast<int>(i));

                    if (ImGui::CollapsingHeader(p.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::Text("Size: %dx%d", p.width, p.height);

                        if (ImGui::ImageButton("##tex", reinterpret_cast<ImTextureID>(p.textureID), ImVec2(previewSize, previewSize)))
                        {
                            sPreviewTexture = p.textureID;
                            sPreviewWidth = p.width;
                            sPreviewHeight = p.height;
                            sPreviewName = p.name;
                            sShowPreview = true;
                        }
                    }

                    ImGui::PopID();
                }
            }
        }
        ImGui::End();

        if (sShowPreview && sPreviewTexture != nullptr)
        {
            ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);

            if (ImGui::Begin(sPreviewName.c_str(), &sShowPreview, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Size: %dx%d", sPreviewWidth, sPreviewHeight);

                float maxSize = 512.0f;
                float scale = 1.0f;
                int maxDim = sPreviewWidth > sPreviewHeight ? sPreviewWidth : sPreviewHeight;
                if (maxDim > maxSize)
                    scale = maxSize / static_cast<float>(maxDim);

                float displayW = sPreviewWidth * scale;
                float displayH = sPreviewHeight * scale;

                ImGui::Image(reinterpret_cast<ImTextureID>(sPreviewTexture), ImVec2(displayW, displayH));
            }
            ImGui::End();
        }
    }
}