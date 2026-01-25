#include "UI_Details.h"
#include "UI_Globals.h"
#include "UI_Outliner.h"
#include "UI_ContentBrowser.h"
#include "UI_Selection.h"
#include "UI_Utils.h"
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"
#include "../models/M3Common.h"
#include "../tex/tex.h"
#include "../Archive.h"

#include "../Skybox/Sky_Manager.h"
#include <imgui.h>
#include <d3d11.h>
#include <set>
#include <algorithm>

namespace UI_Details
{
    void Reset()
    {
    }

    static void DrawM3DetailsForRender(AppState& state, M3Render* render)
    {
        if (!render) return;

        size_t submeshCount = render->getSubmeshCount();
        size_t materialCount = render->getMaterialCount();
        size_t boneCount = render->getBoneCount();
        size_t animCount = render->getAnimationCount();
        size_t texCount = render->getAllTextures().size();

        ImGui::Text("Submeshes: %zu", submeshCount);
        ImGui::Text("Materials: %zu", materialCount);
        ImGui::Text("Bones: %zu", boneCount);
        ImGui::Text("Animations: %zu", animCount);
        ImGui::Text("Textures: %zu", texCount);

        ImGui::Separator();

        bool showSkel = render->getShowSkeleton();
        if (ImGui::Checkbox("Show Skeleton", &showSkel))
            render->setShowSkeleton(showSkel);

        std::set<uint8_t> uniqueGroups;
        for (size_t i = 0; i < submeshCount; ++i)
        {
            uint8_t gid = render->getSubmesh(i).groupId;
            if (gid != 255) uniqueGroups.insert(gid);
        }

        if (uniqueGroups.size() > 1)
        {
            ImGui::Separator();

            int currentVariant = render->getActiveVariant();
            char preview[32];
            if (currentVariant == -1)
                snprintf(preview, sizeof(preview), "All");
            else
                snprintf(preview, sizeof(preview), "%d", currentVariant);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Variant:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);

            if (ImGui::BeginCombo("##VariantCombo", preview))
            {
                bool selAll = (currentVariant == -1);
                if (ImGui::Selectable("All", selAll))
                {
                    render->setActiveVariant(-1);
                    for (size_t i = 0; i < submeshCount; ++i)
                        render->setSubmeshVisible(i, true);
                }

                for (uint8_t gid : uniqueGroups)
                {
                    bool sel = (currentVariant == (int)gid);
                    char label[32];
                    snprintf(label, sizeof(label), "%d", gid);
                    if (ImGui::Selectable(label, sel))
                        render->setActiveVariant((int)gid);
                }

                ImGui::EndCombo();
            }
        }

        if (ImGui::CollapsingHeader("Submeshes"))
        {
            for (size_t i = 0; i < submeshCount; ++i)
            {
                const auto& sm = render->getSubmesh(i);
                ImGui::PushID((int)i);

                bool vis = render->getSubmeshVisible(i);
                if (ImGui::Checkbox("##smvis", &vis))
                    render->setSubmeshVisible(i, vis);
                ImGui::SameLine();

                if (ImGui::TreeNode((void*)(intptr_t)i, "Submesh %zu (Group %u)", i, (unsigned)sm.groupId))
                {
                    ImGui::Text("Material: %u", (unsigned)sm.materialID);
                    ImGui::Text("Vertices: %u", (unsigned)sm.vertexCount);
                    ImGui::Text("Indices: %u", (unsigned)sm.indexCount);
                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
        }

        if (materialCount > 0 && ImGui::CollapsingHeader("Materials"))
        {
            for (size_t i = 0; i < materialCount; ++i)
            {
                ImGui::PushID((int)i);
                if (ImGui::TreeNode((void*)(intptr_t)i, "Material %zu", i))
                {
                    int currentVar = render->getMaterialSelectedVariant(i);
                    int variantCount = (int)render->getMaterialVariantCount(i);
                    if (variantCount > 1)
                    {
                        if (ImGui::SliderInt("Variant", &currentVar, 0, variantCount - 1))
                            render->setMaterialSelectedVariant(i, currentVar);
                    }
                    ImGui::Text("Variants: %d", variantCount);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        if (ImGui::CollapsingHeader("Textures"))
        {
            const auto& textures = render->getAllTextures();
            const auto& srvTextures = render->getGLTextures();
            float thumbSize = 48.0f;
            float windowWidth = ImGui::GetContentRegionAvail().x;
            int columns = std::max(1, (int)(windowWidth / (thumbSize + 8.0f)));

            for (size_t i = 0; i < textures.size(); ++i)
            {
                const auto& tex = textures[i];
                ImGui::PushID((int)i);
                ID3D11ShaderResourceView* srv = (i < srvTextures.size()) ? srvTextures[i].Get() : nullptr;

                if (srv != nullptr)
                {
                    if (ImGui::ImageButton("##texbtn", reinterpret_cast<ImTextureID>(srv), ImVec2(thumbSize, thumbSize)))
                    {
                        int texWidth = 0, texHeight = 0;
                        ID3D11Resource* resource = nullptr;
                        srv->GetResource(&resource);
                        if (resource)
                        {
                            ID3D11Texture2D* texture2D = nullptr;
                            if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture2D)))
                            {
                                D3D11_TEXTURE2D_DESC desc;
                                texture2D->GetDesc(&desc);
                                texWidth = static_cast<int>(desc.Width);
                                texHeight = static_cast<int>(desc.Height);
                                texture2D->Release();
                            }
                            resource->Release();
                        }

                        std::string title = tex.path.empty() ? "Texture Preview" : tex.path;
                        Tex::OpenTexPreviewFromSRV(state, srv, texWidth, texHeight, title);
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("[%zu] %s", i, tex.path.c_str());
                        ImGui::EndTooltip();
                    }
                }
                else
                {
                    ImGui::Button("N/A", ImVec2(thumbSize, thumbSize));
                }

                if ((i + 1) % columns != 0 && i + 1 < textures.size())
                    ImGui::SameLine();

                ImGui::PopID();
            }
        }

        if (boneCount > 0 && ImGui::CollapsingHeader("Bones"))
        {
            const auto& bones = render->getAllBones();
            int selectedBone = render->getSelectedBone();
            static int sLastSelectedBone = -1;

            for (size_t i = 0; i < bones.size(); ++i)
            {
                const auto& bone = bones[i];
                ImGui::PushID((int)i);

                bool isSelected = (selectedBone == (int)i);

                if (isSelected && selectedBone != sLastSelectedBone)
                {
                    ImGui::SetScrollHereY(0.5f);
                    sLastSelectedBone = selectedBone;
                }

                if (isSelected)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));

                char label[128];
                snprintf(label, sizeof(label), "[%zu] %s", i, bone.name.c_str());
                if (ImGui::Selectable(label, isSelected))
                    render->setSelectedBone(isSelected ? -1 : (int)i);

                if (isSelected)
                    ImGui::PopStyleColor();

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Parent: %d", bone.parentId);
                    ImGui::Text("Flags: 0x%04X", bone.flags);
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }

            if (selectedBone < 0)
                sLastSelectedBone = -1;
        }

        if (animCount > 0 && ImGui::CollapsingHeader("Animations"))
        {
            const auto& anims = render->getAllAnimations();
            int playingIdx = render->getPlayingAnimation();

            for (size_t i = 0; i < anims.size(); ++i)
            {
                const auto& anim = anims[i];
                bool isPlaying = (playingIdx == (int)i);
                float duration = (anim.timestampEnd - anim.timestampStart) / 1000.0f;

                char label[64];
                snprintf(label, sizeof(label), "%s Anim %zu (%.2fs)###anim%zu", isPlaying ? ">" : " ", i, duration, i);

                if (isPlaying)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                if (ImGui::Selectable(label, isPlaying))
                {
                    if (isPlaying)
                        render->stopAnimation();
                    else
                        render->playAnimation((int)i);
                }

                if (isPlaying)
                    ImGui::PopStyleColor();
            }
        }
    }

    static void DrawM3Details(AppState& state)
    {
        DrawM3DetailsForRender(state, state.m3Render.get());
    }

    static void DrawPropDetails(AppState& state)
    {
        const Prop* prop = GetSelectedProp();
        if (!prop)
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No prop selected");
            return;
        }

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Selected Prop");
        ImGui::Separator();

        size_t lastSlash = prop->path.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? prop->path.substr(lastSlash + 1) : prop->path;
        ImGui::Text("File: %s", filename.c_str());

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", prop->path.c_str());
            ImGui::EndTooltip();
        }

        ImGui::Text("ID: %u", prop->uniqueID);

        ImGui::Separator();
        ImGui::Text("Position: %.2f, %.2f, %.2f", prop->position.x, prop->position.y, prop->position.z);
        ImGui::Text("Scale: %.3f", prop->scale);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(prop->rotation));
        ImGui::Text("Rotation: %.1f, %.1f, %.1f", euler.x, euler.y, euler.z);

        ImGui::Separator();
        ImGui::Text("unk7 (variant): %d (0x%X)", prop->unk7, prop->unk7);

        ImGui::Text("Color0:");
        ImGui::SameLine();
        ImVec4 c0(prop->color0.r / 255.0f, prop->color0.g / 255.0f, prop->color0.b / 255.0f, prop->color0.a / 255.0f);
        ImGui::ColorButton("##c0", c0, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));

        ImGui::Text("Color1:");
        ImGui::SameLine();
        ImVec4 c1(prop->color1.r / 255.0f, prop->color1.g / 255.0f, prop->color1.b / 255.0f, prop->color1.a / 255.0f);
        ImGui::ColorButton("##c1", c1, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));

        ImGui::Text("Color2:");
        ImGui::SameLine();
        ImVec4 c2(prop->color2.r / 255.0f, prop->color2.g / 255.0f, prop->color2.b / 255.0f, prop->color2.a / 255.0f);
        ImGui::ColorButton("##c2", c2, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));

        if (prop->render)
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Model settings (affects all instances)");
            ImGui::Separator();
            DrawM3DetailsForRender(state, prop->render.get());
        }
    }

    static std::string WideToNarrow(const std::wstring& ws)
    {
        std::string s;
        s.reserve(ws.size());
        for (wchar_t wc : ws) s += static_cast<char>(wc);
        return s;
    }

    static std::string ExtractFilename(const std::string& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            return path.substr(lastSlash + 1);
        return path;
    }

    static void DrawSkyModelDetails(AppState& state)
    {
        auto& skyMgr = Sky::Manager::Instance();

        if (skyMgr.isLoading()) return;

        M3Render* render = skyMgr.getSelectedSkyModel();
        if (!render) return;

        auto paths = skyMgr.getSkyModelPaths();
        int idx = skyMgr.getSelectedSkyModelIndex();

        std::string name = "Sky Model";
        if (idx >= 0 && idx < static_cast<int>(paths.size()))
        {
            name = ExtractFilename(WideToNarrow(paths[idx]));
        }

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", name.c_str());
        ImGui::Separator();

        DrawM3DetailsForRender(state, render);
    }

    static void DrawAnimationPlayback(AppState& state)
    {
        M3Render* render = nullptr;

        if (state.m3Render && state.m3Render->isAnimationPlaying())
        {
            render = state.m3Render.get();
        }
        else
        {
            const Prop* prop = GetSelectedProp();
            if (prop && prop->render && prop->render->isAnimationPlaying())
            {
                render = prop->render.get();
            }
        }

        if (!render) return;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        float sidebarWidth = UI_Outliner::GetSidebarWidth();
        float playbackWidth = 180.0f;
        float popupX = vp->Pos.x + vp->Size.x - sidebarWidth - playbackWidth - 20.0f;
        float popupY = vp->Pos.y + 40.0f;

        ImGui::SetNextWindowPos(ImVec2(popupX, popupY), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;

        bool showPlayback = true;
        if (ImGui::Begin("Playback", &showPlayback, popupFlags))
        {
            float duration = render->getAnimationDuration();
            float currentTime = render->getAnimationTime();
            bool isPaused = render->isAnimationPaused();

            ImGui::ProgressBar(duration > 0 ? currentTime / duration : 0, ImVec2(160, 14));

            float btnWidth = 50.0f;
            if (isPaused)
            {
                if (ImGui::Button("Play", ImVec2(btnWidth, 0)))
                    render->resumeAnimation();
            }
            else
            {
                if (ImGui::Button("Pause", ImVec2(btnWidth, 0)))
                    render->pauseAnimation();
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop", ImVec2(btnWidth, 0)))
                render->stopAnimation();
        }
        ImGui::End();
        ImGui::PopStyleVar();

        if (!showPlayback)
            render->stopAnimation();
    }

    static void DrawAreaSelectionDetails(AppState& state)
    {
        if (gSelectedPropID != 0)
        {
            DrawPropDetails(state);
            return;
        }

        if (gSelectedAreaIndex < 0 || gSelectedAreaIndex >= static_cast<int>(gLoadedAreas.size()))
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select an item in the Outliner");
            return;
        }

        const auto& area = gLoadedAreas[gSelectedAreaIndex];
        if (!area)
        {
            ImGui::Text("Invalid area");
            return;
        }

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Selected Terrain");
        ImGui::Separator();

        ImGui::Text("Tile: %d, %d", area->getTileX(), area->getTileY());

        auto offset = area->getWorldOffset();
        ImGui::Text("World Offset: %.1f, %.1f, %.1f", offset.x, offset.y, offset.z);

        auto minB = area->getMinBounds();
        auto maxB = area->getMaxBounds();
        ImGui::Text("Bounds Min: %.1f, %.1f, %.1f", minB.x, minB.y, minB.z);
        ImGui::Text("Bounds Max: %.1f, %.1f, %.1f", maxB.x, maxB.y, maxB.z);

        const auto& chunks = area->getChunks();
        ImGui::Text("Chunks: %zu", chunks.size());

        size_t propCount = area->getPropCount();
        size_t loadedProps = area->getLoadedPropCount();
        ImGui::Text("Props: %zu / %zu loaded", loadedProps, propCount);

        if (loadedProps < propCount)
        {
            if (ImGui::Button("Load Props", ImVec2(-1, 24)))
            {
                area->loadAllProps();
            }
        }
    }

    void Draw(AppState& state)
    {
        HandleGlobalKeys(state);

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float sidebarWidth = UI_Outliner::GetSidebarWidth();
        float outlinerHeight = UI_Outliner::GetWindowHeight();
        float contentBrowserHeight = UI_ContentBrowser::GetHeight();

        float topY = viewport->Pos.y + outlinerHeight;
        float availableHeight = viewport->Size.y - outlinerHeight - contentBrowserHeight;
        float startX = viewport->Pos.x + viewport->Size.x - sidebarWidth;

        ImGui::SetNextWindowPos(ImVec2(startX, topY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth, availableHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.95f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        if (ImGui::Begin("Details", nullptr, flags))
        {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(startX, topY));

            ImGui::InvisibleButton("##DetailsResizeW", ImVec2(5.0f, availableHeight));
            if (ImGui::IsItemActive())
            {
                float newWidth = sidebarWidth - ImGui::GetIO().MouseDelta.x;
                UI_Outliner::SetSidebarWidth(newWidth);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            ImGui::SetCursorScreenPos(cursorPos);

            ImGui::BeginChild("##DetailsScroll", ImVec2(0, 0), false);

            if (gLoadedModel)
            {
                DrawM3Details(state);
            }
            else if (gSelectedSkyModelIndex >= 0)
            {
                DrawSkyModelDetails(state);
            }
            else if (!gLoadedAreas.empty())
            {
                DrawAreaSelectionDetails(state);
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No content loaded");
                ImGui::Spacing();
                ImGui::TextWrapped("Use the Content Browser to load .area or .m3 files.");
            }

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();

        DrawAnimationPlayback(state);
    }
}