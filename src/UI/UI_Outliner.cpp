#include "UI_Outliner.h"
#include "UI_Globals.h"
#include "UI_Utils.h"
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <algorithm>

namespace UI_Outliner
{
    static int sSelectedIndex = -1;
    static float sWindowHeight = 300.0f;
    static float sSidebarWidth = 300.0f;
    static std::vector<std::string> sCachedNames;
    static bool sNeedsRefresh = true;

    struct OutlinerEntry
    {
        std::string name;
        int areaIndex;
        int propIndex;
        bool isArea;
        bool isProp;
        bool isM3;
    };

    static std::vector<OutlinerEntry> sEntries;

    static std::string ExtractFilename(const std::string& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            return path.substr(lastSlash + 1);
        return path;
    }

    static void RefreshEntries()
    {
        sEntries.clear();

        if (gLoadedModel)
        {
            OutlinerEntry entry;
            entry.name = ExtractFilename(gLoadedModel->getModelName());
            entry.areaIndex = -1;
            entry.propIndex = -1;
            entry.isArea = false;
            entry.isProp = false;
            entry.isM3 = true;
            sEntries.push_back(entry);
        }
        else if (!gLoadedAreas.empty())
        {
            for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
            {
                const auto& area = gLoadedAreas[areaIdx];
                if (!area) continue;

                OutlinerEntry areaEntry;
                areaEntry.name = "Area_" + std::to_string(area->getTileX()) + "_" + std::to_string(area->getTileY());
                areaEntry.areaIndex = static_cast<int>(areaIdx);
                areaEntry.propIndex = -1;
                areaEntry.isArea = true;
                areaEntry.isProp = false;
                areaEntry.isM3 = false;
                sEntries.push_back(areaEntry);

                const auto& props = area->getProps();
                for (size_t propIdx = 0; propIdx < props.size(); ++propIdx)
                {
                    const auto& prop = props[propIdx];
                    OutlinerEntry propEntry;
                    propEntry.name = ExtractFilename(prop.path);
                    if (propEntry.name.empty())
                        propEntry.name = "Prop_" + std::to_string(prop.uniqueID);
                    propEntry.areaIndex = static_cast<int>(areaIdx);
                    propEntry.propIndex = static_cast<int>(propIdx);
                    propEntry.isArea = false;
                    propEntry.isProp = true;
                    propEntry.isM3 = false;
                    sEntries.push_back(propEntry);
                }
            }
        }

        sNeedsRefresh = false;
    }

    void Reset()
    {
        sSelectedIndex = -1;
        sEntries.clear();
        sNeedsRefresh = true;
    }

    int GetSelectedIndex() { return sSelectedIndex; }
    void SetSelectedIndex(int index) { sSelectedIndex = index; }

    float GetWindowHeight() { return sWindowHeight; }
    void SetWindowHeight(float height) { sWindowHeight = height; }
    float GetSidebarWidth() { return sSidebarWidth; }
    void SetSidebarWidth(float width) { sSidebarWidth = width; }

    void Draw(AppState& state)
{
    static size_t sLastAreaCount = 0;
    static size_t sLastPropCount = 0;
    static bool sLastHadModel = false;

    size_t currentPropCount = 0;
    for (const auto& area : gLoadedAreas)
    {
        if (area) currentPropCount += area->getProps().size();
    }

    bool hasModel = (gLoadedModel != nullptr);

    if (gLoadedAreas.size() != sLastAreaCount ||
        currentPropCount != sLastPropCount ||
        hasModel != sLastHadModel)
    {
        sNeedsRefresh = true;
        sLastAreaCount = gLoadedAreas.size();
        sLastPropCount = currentPropCount;
        sLastHadModel = hasModel;
    }

    if (sNeedsRefresh)
        RefreshEntries();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (sSidebarWidth < 200.0f) sSidebarWidth = 200.0f;
    if (sSidebarWidth > viewport->Size.x - 50.0f) sSidebarWidth = viewport->Size.x - 50.0f;

    float startX = viewport->Pos.x + viewport->Size.x - sSidebarWidth;
    float startY = viewport->Pos.y;

    ImGui::SetNextWindowPos(ImVec2(startX, startY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(sSidebarWidth, sWindowHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.95f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    if (ImGui::Begin("Outliner", nullptr, flags))
    {
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(startX, startY));

        ImGui::InvisibleButton("##OutlinerResizeW", ImVec2(5.0f, sWindowHeight));
        if (ImGui::IsItemActive())
        {
            sSidebarWidth -= ImGui::GetIO().MouseDelta.x;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        ImGui::SetCursorScreenPos(cursorPos);

        ImGui::BeginChild("##OutlinerScroll", ImVec2(0, 0), false);

        if (sEntries.empty())
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No content loaded");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "Open Content Browser");
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "to load files");
        }
        else
        {
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            availSize.y -= 5.0f;

            if (ImGui::BeginChild("OutlinerList", ImVec2(0, availSize.y), false))
            {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(sEntries.size()));

                while (clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                    {
                        const auto& entry = sEntries[i];

                        ImGui::PushID(i);

                        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        if (entry.isArea)
                            color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                        else if (entry.isProp)
                            color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                        else if (entry.isM3)
                            color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);

                        bool isSelected = (sSelectedIndex == i);

                        if (entry.isProp)
                        {
                            ImGui::Indent(20.0f);
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, color);

                        if (ImGui::Selectable(entry.name.c_str(), isSelected))
                        {
                            sSelectedIndex = i;
                            if (entry.isArea)
                            {
                                gSelectedAreaIndex = entry.areaIndex;
                            }
                        }

                        ImGui::PopStyleColor();

                        if (entry.isProp)
                        {
                            ImGui::Unindent(20.0f);
                        }

                        ImGui::PopID();
                    }
                }
                clipper.End();
            }
            ImGui::EndChild();
        }

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 5.0f);
        ImGui::InvisibleButton("##OutlinerSplitter", ImVec2(-1, 5.0f));
        if (ImGui::IsItemActive())
        {
            sWindowHeight += ImGui::GetIO().MouseDelta.y;
            if (sWindowHeight < 50.0f) sWindowHeight = 50.0f;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }

        ImU32 splitterColor = ImGui::IsItemHovered() || ImGui::IsItemActive() ? IM_COL32(100, 149, 237, 255) : IM_COL32(40, 40, 50, 255);
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(startX, startY + sWindowHeight - 1.0f),
            ImVec2(startX + sSidebarWidth, startY + sWindowHeight - 1.0f),
            splitterColor, 1.0f);

        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
}