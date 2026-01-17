#include "UI_Models.h"
#include "imgui.h"
#include "../models/M3Render.h"
#include "../models/M3Common.h"
#include "UI.h"
#include <algorithm>

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

        std::string windowTitle = "Model";
        if (render && !render->getModelName().empty())
            windowTitle = FormatModelName(render->getModelName());

        if (ImGui::Begin(windowTitle.c_str(), &state.show_models_window))
        {
            if (!render)
            {
                ImGui::TextUnformatted("No model loaded.");
                ImGui::End();
                return;
            }

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
            {
                render->setShowSkeleton(showSkel);
            }

            ImGui::Separator();

            const auto& submeshGroups = render->getSubmeshGroups();
            if (!submeshGroups.empty())
            {
                ImGui::Text("Variant:");
                ImGui::SameLine();

                int currentVariant = render->getActiveVariant();

                if (ImGui::RadioButton("All", currentVariant == -1))
                {
                    render->setActiveVariant(-1);
                }

                for (size_t i = 0; i < submeshGroups.size(); ++i)
                {
                    ImGui::SameLine();
                    char label[32];
                    snprintf(label, sizeof(label), "%d", submeshGroups[i].submeshId);
                    if (ImGui::RadioButton(label, currentVariant == (int)i))
                    {
                        render->setActiveVariant((int)i);
                    }
                }

                ImGui::Separator();
            }

            if (ImGui::CollapsingHeader("Submeshes"))
            {
                for (size_t i = 0; i < submeshCount; ++i)
                {
                    const auto& sm = render->getSubmesh(i);
                    ImGui::PushID(static_cast<int>(i));

                    bool visible = render->getSubmeshVisible(i);
                    if (ImGui::Checkbox("##vis", &visible))
                    {
                        render->setSubmeshVisible(i, visible);
                    }
                    ImGui::SameLine();

                    if (ImGui::TreeNode((void*)(intptr_t)i, "Submesh %zu (Group %d)", i, sm.groupId))
                    {
                        ImGui::Text("Material ID: %d", sm.materialID);
                        ImGui::Text("Start Index: %u", sm.startIndex);
                        ImGui::Text("Index Count: %u", sm.indexCount);
                        ImGui::Text("Start Vertex: %u", sm.startVertex);
                        ImGui::Text("Vertex Count: %u", sm.vertexCount);
                        ImGui::Text("Group ID: %d", sm.groupId);
                        ImGui::Text("Bone Mapping: %u - %u", sm.startBoneMapping, sm.startBoneMapping + sm.nrBoneMapping);
                        ImGui::Text("Bound Min: (%.2f, %.2f, %.2f)", sm.boundMin.x, sm.boundMin.y, sm.boundMin.z);
                        ImGui::Text("Bound Max: (%.2f, %.2f, %.2f)", sm.boundMax.x, sm.boundMax.y, sm.boundMax.z);
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
                            {
                                render->setMaterialSelectedVariant(i, currentVariant);
                            }
                        }

                        ImGui::Text("Variants: %d", variantCount);

                        const auto& mat = render->getAllMaterials()[i];
                        for (size_t v = 0; v < mat.variants.size(); v++)
                        {
                            const auto& var = mat.variants[v];
                            if (ImGui::TreeNode((void*)(intptr_t)(i * 1000 + v), "Variant %zu", v))
                            {
                                ImGui::Text("Texture A: %d", var.textureIndexA);
                                ImGui::Text("Texture B: %d", var.textureIndexB);
                                if (!var.textureColorPath.empty())
                                    ImGui::Text("Color: %s", var.textureColorPath.c_str());
                                if (!var.textureNormalPath.empty())
                                    ImGui::Text("Normal: %s", var.textureNormalPath.c_str());
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Textures"))
            {
                const auto& textures = render->getAllTextures();
                const auto& glTextures = render->getGLTextures();

                float thumbSize = 64.0f;
                float windowWidth = ImGui::GetContentRegionAvail().x;
                int columns = std::max(1, (int)(windowWidth / (thumbSize + 10.0f)));

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
                            ImGui::Text("Type: %s", tex.textureType.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    else
                    {
                        ImGui::Button("N/A", ImVec2(thumbSize, thumbSize));
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("[%zu] %s (not loaded)", i, tex.path.c_str());
                            ImGui::EndTooltip();
                        }
                    }

                    if ((i + 1) % columns != 0 && i + 1 < textures.size())
                        ImGui::SameLine();

                    ImGui::PopID();
                }
            }

            if (showTexturePopup && selectedTextureIndex >= 0)
            {
                const auto& textures = render->getAllTextures();
                const auto& glTextures = render->getGLTextures();

                if (selectedTextureIndex < (int)textures.size())
                {
                    const auto& tex = textures[selectedTextureIndex];
                    std::string popupTitle = tex.path;
                    if (popupTitle.empty())
                        popupTitle = "Texture " + std::to_string(selectedTextureIndex);

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
                        ImGui::Text("Intensity: %.2f", tex.intensity);
                        ImGui::Text("Flags: 0x%08X", tex.flags);
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

            if (ImGui::CollapsingHeader("Bones"))
            {
                const auto& bones = render->getAllBones();
                for (size_t i = 0; i < bones.size(); ++i)
                {
                    const auto& bone = bones[i];
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::TreeNode((void*)(intptr_t)i, "[%d] %s", bone.id, bone.name.c_str()))
                    {
                        ImGui::Text("Parent ID: %d", bone.parentId);
                        ImGui::Text("Global ID: %d", bone.globalId);
                        ImGui::Text("Flags: 0x%04X", bone.flags);
                        ImGui::Text("Position: (%.3f, %.3f, %.3f)", bone.position.x, bone.position.y, bone.position.z);
                        ImGui::Text("Path: %s", bone.parentPath.c_str());

                        if (ImGui::TreeNode("Animation Tracks"))
                        {
                            for (int t = 0; t < 8; t++)
                            {
                                const auto& track = bone.tracks[t];
                                if (!track.keyframes.empty())
                                {
                                    ImGui::Text("Track %d: %zu keyframes", t + 1, track.keyframes.size());
                                }
                            }
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Global Matrix"))
                        {
                            const auto& m = bone.globalMatrix;
                            ImGui::Text("%.3f %.3f %.3f %.3f", m[0][0], m[0][1], m[0][2], m[0][3]);
                            ImGui::Text("%.3f %.3f %.3f %.3f", m[1][0], m[1][1], m[1][2], m[1][3]);
                            ImGui::Text("%.3f %.3f %.3f %.3f", m[2][0], m[2][1], m[2][2], m[2][3]);
                            ImGui::Text("%.3f %.3f %.3f %.3f", m[3][0], m[3][1], m[3][2], m[3][3]);
                            ImGui::TreePop();
                        }

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Animations"))
            {
                const auto& anims = render->getAllAnimations();
                for (size_t i = 0; i < anims.size(); ++i)
                {
                    const auto& anim = anims[i];
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::TreeNode((void*)(intptr_t)i, "Animation %zu (Seq: %d)", i, anim.sequenceId))
                    {
                        ImGui::Text("Sequence ID: %d", anim.sequenceId);
                        ImGui::Text("Fallback: %d", anim.fallbackSequence);
                        float startSec = anim.timestampStart / 1000.0f;
                        float endSec = anim.timestampEnd / 1000.0f;
                        ImGui::Text("Time: %.3fs - %.3fs (%.3fs)", startSec, endSec, endSec - startSec);
                        ImGui::Text("Frames: %u - %u", anim.timestampStart, anim.timestampEnd);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::End();
    }
}