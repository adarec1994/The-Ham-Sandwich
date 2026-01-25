#include "UI_Sky.h"
#include "../Skybox/Sky_Manager.h"
#include "imgui.h"
#include <cstdio>

namespace UI {

static std::string WideToNarrow(const std::wstring& ws)
{
    std::string s;
    s.reserve(ws.size());
    for (wchar_t wc : ws) s += static_cast<char>(wc);
    return s;
}

void DrawSkyPanel()
{
    auto& mgr = Sky::Manager::Instance();

    if (ImGui::Begin("Sky"))
    {
        if (!mgr.hasSky())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No sky loaded");
            ImGui::Separator();
        }

        const auto& skyMap = mgr.getSkyIDToPath();
        const auto& collectedIDs = mgr.getCollectedSkyIDs();

        if (!collectedIDs.empty())
        {
            ImGui::Text("Area Sky IDs:");
            for (uint32_t id : collectedIDs)
            {
                auto it = skyMap.find(id);

                if (it != skyMap.end())
                {
                    const char* filename = strrchr(it->second.c_str(), '/');
                    if (!filename) filename = strrchr(it->second.c_str(), '\\');
                    if (!filename) filename = it->second.c_str();
                    else filename++;

                    ImGui::Text("%u: %s", id, filename);
                    ImGui::SameLine();

                    char btnLabel[32];
                    snprintf(btnLabel, sizeof(btnLabel), "Load##%u", id);
                    if (ImGui::SmallButton(btnLabel))
                    {
                        mgr.loadSkyFromID(id);
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%u: (not in table)", id);
                }
            }
            ImGui::Separator();
        }

        float hours = mgr.getTimeOfDayHours();
        if (ImGui::SliderFloat("Time of Day", &hours, 0.0f, 24.0f, "%.1f h"))
        {
            mgr.setTimeOfDayHours(hours);
        }

        if (ImGui::Button("Dawn")) mgr.setTimeOfDayHours(6.0f);
        ImGui::SameLine();
        if (ImGui::Button("Noon")) mgr.setTimeOfDayHours(12.0f);
        ImGui::SameLine();
        if (ImGui::Button("Dusk")) mgr.setTimeOfDayHours(18.0f);
        ImGui::SameLine();
        if (ImGui::Button("Midnight")) mgr.setTimeOfDayHours(0.0f);

        ImGui::Separator();

        if (mgr.hasSky())
        {
            const Sky::File* sky = mgr.getActiveSky();
            if (sky)
            {
                ImGui::Text("Version: %u", sky->getVersion());

                const auto& envMap = sky->getEnvironmentMap();
                if (!envMap.empty())
                {
                    ImGui::Text("Env Map: %s", WideToNarrow(envMap).c_str());
                }

                const auto& lut = sky->getLutFile();
                if (!lut.empty())
                {
                    ImGui::Text("LUT: %s", WideToNarrow(lut).c_str());
                }

                ImGui::Separator();
                ImGui::Text("Current Lighting:");

                glm::vec4 sunColor = mgr.getCurrentSunColor();
                ImGui::ColorEdit4("Sun Color", &sunColor.x, ImGuiColorEditFlags_NoInputs);

                Sky::FogSettings fog = mgr.getCurrentFog();
                ImGui::Text("Fog: start=%.1f", fog.fogStartDistance);

                Sky::PostFXSettings postfx = mgr.getCurrentPostFX();
                ImGui::Text("PostFX: sat=%.2f bright=%.2f", postfx.saturation, postfx.brightness);
            }
        }
    }
    ImGui::End();
}

}