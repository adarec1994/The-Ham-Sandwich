#include "UI_Models.h"
#include "imgui.h"
#include "../models/M3Render.h"
#include "../models/M3Common.h"
#include "UI.h"
#include <algorithm>
#include <set>

namespace UI_Models
{
    static std::string FormatModelName(const std::string& name)
    {
        std::string result = name;
        if (result.size() > 3 && result.substr(result.size() - 3) == ".m3")
            result = result.substr(0, result.size() - 3);
        std::replace(result.begin(), result.end(), '_', ' ');
        return result;
    }

    static int selectedTextureIndex = -1;
    static bool showTexturePopup = false;

    void Draw(AppState& state)
    {
        if (!state.show_models_window) return;

        M3Render* render = state.m3Render.get();
        if (!render) return;

        std::string windowTitle = "Model";
        if (!render->getModelName().empty())
            windowTitle = FormatModelName(render->getModelName());

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float windowWidth = 300.0f;
        float windowX = viewport->Pos.x + viewport->Size.x - windowWidth - 10.0f;
        float windowY = viewport->Pos.y + 40.0f;

        ImGui::SetNextWindowPos(ImVec2(windowX, windowY), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(250, 0), ImVec2(450, viewport->Size.y - 60.0f));
        ImGui::SetNextWindowBgAlpha(0.9f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin(windowTitle.c_str(), &state.show_models_window, flags))
        {
            size_t submeshCount = render->getSubmeshCount();
            size_t materialCount = render->getMaterialCount();
            size_t boneCount = render->getBoneCount();
            size_t animCount = render->getAnimationCount();
            size_t texCount = render->getAllTextures().size();

            ImGui::Text("Submeshes: %zu  Materials: %zu", submeshCount, materialCount);
            ImGui::Text("Bones: %zu  Animations: %zu  Textures: %zu", boneCount, animCount, texCount);

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
                ImGui::Text("Variant:");
                int currentVariant = render->getActiveVariant();
                float availWidth = ImGui::GetContentRegionAvail().x;
                float btnWidth = 45.0f;
                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float x = 0.0f;

                for (uint8_t gid : uniqueGroups)
                {
                    if (x + btnWidth > availWidth && x > 0)
                    {
                        x = 0.0f;
                    }
                    else if (x > 0)
                    {
                        ImGui::SameLine();
                    }

                    char label[32];
                    snprintf(label, sizeof(label), "%d", gid);
                    if (ImGui::RadioButton(label, currentVariant == (int)gid))
                        render->setActiveVariant((int)gid);

                    x += btnWidth + spacing;
                }
            }

            if (ImGui::CollapsingHeader("Animations"))
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

                    if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                    if (ImGui::Selectable(label, isPlaying))
                    {
                        if (isPlaying) render->stopAnimation();
                        else render->playAnimation((int)i);
                    }
                    if (isPlaying) ImGui::PopStyleColor();
                }
            }

            if (ImGui::CollapsingHeader("Submeshes"))
            {
                for (size_t i = 0; i < submeshCount; ++i)
                {
                    const auto& sm = render->getSubmesh(i);
                    ImGui::PushID(static_cast<int>(i));

                    bool visible = render->getSubmeshVisible(i);
                    if (ImGui::Checkbox("##vis", &visible))
                        render->setSubmeshVisible(i, visible);
                    ImGui::SameLine();

                    if (ImGui::TreeNode((void*)(intptr_t)i, "Submesh %zu (Group %d)", i, sm.groupId))
                    {
                        ImGui::Text("Material: %d  Indices: %u  Vertices: %u", sm.materialID, sm.indexCount, sm.vertexCount);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Materials"))
            {
                for (size_t i = 0; i < materialCount; ++i)
                {
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::TreeNode((void*)(intptr_t)i, "Material %zu", i))
                    {
                        int currentVariant = render->getMaterialSelectedVariant(i);
                        int variantCount = (int)render->getMaterialVariantCount(i);
                        if (variantCount > 1)
                        {
                            if (ImGui::SliderInt("Variant", &currentVariant, 0, variantCount - 1))
                                render->setMaterialSelectedVariant(i, currentVariant);
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
                const auto& glTextures = render->getGLTextures();
                float thumbSize = 48.0f;
                float windowWidth = ImGui::GetContentRegionAvail().x;
                int columns = std::max(1, (int)(windowWidth / (thumbSize + 8.0f)));

                for (size_t i = 0; i < textures.size(); ++i)
                {
                    const auto& tex = textures[i];
                    ImGui::PushID(static_cast<int>(i));
                    unsigned int glTex = (i < glTextures.size()) ? glTextures[i] : 0;

                    if (glTex != 0)
                    {
                        if (ImGui::ImageButton("##texbtn", (ImTextureID)(uintptr_t)glTex, ImVec2(thumbSize, thumbSize)))
                        {
                            selectedTextureIndex = static_cast<int>(i);
                            showTexturePopup = true;
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

            if (ImGui::CollapsingHeader("Bones"))
            {
                const auto& bones = render->getAllBones();
                for (size_t i = 0; i < bones.size(); ++i)
                {
                    const auto& bone = bones[i];
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::TreeNode((void*)(intptr_t)i, "[%d] %s", bone.id, bone.name.c_str()))
                    {
                        ImGui::Text("Parent: %d  Flags: 0x%04X", bone.parentId, bone.flags);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::End();

        if (showTexturePopup && selectedTextureIndex >= 0)
        {
            const auto& textures = render->getAllTextures();
            const auto& glTextures = render->getGLTextures();

            if (selectedTextureIndex < (int)textures.size())
            {
                const auto& tex = textures[selectedTextureIndex];
                std::string popupTitle = tex.path.empty() ? "Texture " + std::to_string(selectedTextureIndex) : tex.path;

                ImGui::SetNextWindowSize(ImVec2(520, 550), ImGuiCond_FirstUseEver);
                if (ImGui::Begin(popupTitle.c_str(), &showTexturePopup))
                {
                    unsigned int glTex = (selectedTextureIndex < (int)glTextures.size()) ? glTextures[selectedTextureIndex] : 0;
                    if (glTex != 0)
                    {
                        ImVec2 avail = ImGui::GetContentRegionAvail();
                        float maxDim = std::min(avail.x, avail.y - 60.0f);
                        ImGui::Image((ImTextureID)(uintptr_t)glTex, ImVec2(maxDim, maxDim));
                    }
                    ImGui::Separator();
                    ImGui::Text("Path: %s", tex.path.c_str());
                    ImGui::Text("Type: %s (%d)", tex.textureType.c_str(), tex.type);
                }
                ImGui::End();

                if (!showTexturePopup)
                    selectedTextureIndex = -1;
            }
            else
            {
                showTexturePopup = false;
                selectedTextureIndex = -1;
            }
        }

        if (render->isAnimationPlaying())
        {
            ImGuiViewport* vp = ImGui::GetMainViewport();
            float modelInfoX = vp->Pos.x + vp->Size.x - 300.0f - 10.0f;
            float playbackWidth = 180.0f;
            float popupX = modelInfoX - playbackWidth - 10.0f;
            float popupY = vp->Pos.y + 40.0f;

            ImGui::SetNextWindowPos(ImVec2(popupX, popupY), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowBgAlpha(0.9f);

            ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;

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

            if (!showPlayback)
                render->stopAnimation();
        }
    }
}