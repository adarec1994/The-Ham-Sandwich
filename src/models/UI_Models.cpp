#include "UI_models.h"
#include "imgui.h"
#include "M3Render.h"
#include "../UI/UI.h"

namespace UI_Models
{
    void Draw(AppState& state)
    {
        if (!state.show_models_window) return;

        M3Render* render = state.m3Render.get();

        if (ImGui::Begin("Model", &state.show_models_window))
        {
            if (!render)
            {
                ImGui::TextUnformatted("No model loaded.");
                ImGui::End();
                return;
            }

            size_t submeshCount = render->getSubmeshCount();
            size_t materialCount = render->getMaterialCount();

            ImGui::Text("Submeshes: %d", (int)submeshCount);
            ImGui::Text("Materials: %d", (int)materialCount);

            if (ImGui::CollapsingHeader("Submeshes", ImGuiTreeNodeFlags_DefaultOpen))
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

                    if (ImGui::TreeNode((void*)(intptr_t)i, "Submesh %zu", i))
                    {
                        ImGui::Text("materialID: %d", sm.materialID);
                        ImGui::Text("startIndex: %u", sm.startIndex);
                        ImGui::Text("indexCount: %u", sm.indexCount);
                        ImGui::Text("startVertex: %u", sm.startVertex);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
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
                        else
                        {
                            ImGui::Text("No variants available.");
                        }

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::End();
    }
}