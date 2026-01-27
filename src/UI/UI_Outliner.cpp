#include "UI_Outliner.h"
#include "UI_Globals.h"
#include "UI_Selection.h"
#include "UI_Utils.h"
#include "UI_ContentBrowser.h"
#include "UI_TopBar.h"
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"
#include "../Skybox/Sky_Manager.h"

#include <imgui.h>
#include <vector>
#include <string>
#include <algorithm>

namespace UI_Outliner
{
    static float sWindowHeight = 300.0f;
    static float sSidebarWidth = 300.0f;

    static std::string ExtractFilename(const std::string& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            return path.substr(lastSlash + 1);
        return path;
    }

    static std::string ExtractFolderPath(const std::string& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            return path.substr(0, lastSlash);
        return "";
    }

    static std::string WideToNarrow(const std::wstring& ws)
    {
        std::string s;
        s.reserve(ws.size());
        for (wchar_t wc : ws) s += static_cast<char>(wc);
        return s;
    }

    void Reset()
    {
    }

    float GetWindowHeight() { return sWindowHeight; }
    void SetWindowHeight(float height) { sWindowHeight = height; }
    float GetSidebarWidth() { return sSidebarWidth; }
    void SetSidebarWidth(float width) { sSidebarWidth = width; }

    void Draw(AppState& state)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        if (sSidebarWidth < 200.0f) sSidebarWidth = 200.0f;
        if (sSidebarWidth > viewport->Size.x - 50.0f) sSidebarWidth = viewport->Size.x - 50.0f;

        float topBarHeight = UI_TopBar::GetHeight();
        float startX = viewport->Pos.x + viewport->Size.x - sSidebarWidth;
        float startY = viewport->Pos.y + topBarHeight;

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

            ImGui::BeginChild("##OutlinerScroll", ImVec2(0, -5.0f), false);

            if (gLoadedModel)
            {
                std::string modelName = ExtractFilename(gLoadedModel->getModelName());
                std::string fullPath = gLoadedModel->getModelName();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                ImGui::Selectable(modelName.c_str(), false);
                ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem())
                {
                    if (!fullPath.empty())
                    {
                        if (ImGui::MenuItem("Browse to Folder"))
                        {
                            std::string folderPath = ExtractFolderPath(fullPath);
                            if (!folderPath.empty())
                            {
                                UI_ContentBrowser::NavigateToPath(state, folderPath);
                            }
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("No path available");
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::IsItemHovered() && !fullPath.empty())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", fullPath.c_str());
                    ImGui::EndTooltip();
                }
            }
            else if (!gLoadedAreas.empty())
            {
                auto& skyMgr = Sky::Manager::Instance();

                if (!skyMgr.isLoading())
                {
                    size_t skyCount = skyMgr.getSkyboxM3Count();
                    auto skyPaths = skyMgr.getSkyModelPaths();

                    if (skyCount > 0)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                        bool skyOpen = ImGui::TreeNodeEx("Sky Models", ImGuiTreeNodeFlags_DefaultOpen);
                        ImGui::PopStyleColor();

                        if (skyOpen)
                        {
                            for (size_t i = 0; i < skyCount; i++)
                            {
                                ImGui::PushID(static_cast<int>(i) + 10000);

                                std::string path = (i < skyPaths.size()) ? WideToNarrow(skyPaths[i]) : "";
                                std::string name = ExtractFilename(path);
                                if (name.empty())
                                    name = "SkyModel_" + std::to_string(i);

                                bool isSelected = IsSkyModelSelected(static_cast<int>(i));

                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                                if (ImGui::Selectable(name.c_str(), isSelected))
                                {
                                    SelectSkyModel(static_cast<int>(i));
                                }
                                ImGui::PopStyleColor();

                                if (ImGui::BeginPopupContextItem())
                                {
                                    if (!path.empty())
                                    {
                                        if (ImGui::MenuItem("Browse to Folder"))
                                        {
                                            std::string folderPath = ExtractFolderPath(path);
                                            if (!folderPath.empty())
                                            {
                                                UI_ContentBrowser::NavigateToPath(state, folderPath);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ImGui::TextDisabled("No path available");
                                    }
                                    ImGui::EndPopup();
                                }

                                if (ImGui::IsItemHovered() && !path.empty())
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::Text("%s", path.c_str());
                                    ImGui::EndTooltip();
                                }

                                ImGui::PopID();
                            }
                            ImGui::TreePop();
                        }
                    }
                }

                for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
                {
                    const auto& area = gLoadedAreas[areaIdx];
                    if (!area) continue;

                    ImGui::PushID(static_cast<int>(areaIdx));

                    std::string areaName = "Area_" + std::to_string(area->getTileX()) + "_" + std::to_string(area->getTileY());
                    bool areaSelected = (gSelectedAreaIndex == static_cast<int>(areaIdx) && gSelectedPropID == 0 && gSelectedSkyModelIndex < 0);

                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                    ImGuiTreeNodeFlags areaFlags = ImGuiTreeNodeFlags_DefaultOpen;
                    if (areaSelected)
                        areaFlags |= ImGuiTreeNodeFlags_Selected;

                    bool areaOpen = ImGui::TreeNodeEx(areaName.c_str(), areaFlags);
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    {
                        ClearPropSelection();
                        ClearSkyModelSelection();
                        gSelectedAreaIndex = static_cast<int>(areaIdx);
                    }

                    if (areaOpen)
                    {
                        const auto& props = area->getProps();
                        for (size_t propIdx = 0; propIdx < props.size(); ++propIdx)
                        {
                            const auto& prop = props[propIdx];

                            ImGui::PushID(static_cast<int>(propIdx));

                            bool isHidden = IsPropHidden(prop.uniqueID);
                            bool isDeleted = IsPropDeleted(prop.uniqueID);

                            std::string propName = ExtractFilename(prop.path);
                            if (propName.empty())
                                propName = "Prop_" + std::to_string(prop.uniqueID);

                            if (isHidden)
                                propName = "[H] " + propName;
                            if (isDeleted)
                                propName = "[X] " + propName;

                            ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                            if (isDeleted)
                                color = ImVec4(0.5f, 0.2f, 0.2f, 0.6f);
                            else if (isHidden)
                                color = ImVec4(0.5f, 0.5f, 0.5f, 0.6f);

                            bool isSelected = IsPropSelected(prop.uniqueID);

                            ImGui::PushStyleColor(ImGuiCol_Text, color);
                            if (ImGui::Selectable(propName.c_str(), isSelected))
                            {
                                if (!isDeleted)
                                {
                                    SelectProp(prop.uniqueID, static_cast<int>(areaIdx));
                                }
                            }
                            ImGui::PopStyleColor();

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (!prop.path.empty())
                                {
                                    if (ImGui::MenuItem("Browse to Folder"))
                                    {
                                        std::string folderPath = ExtractFolderPath(prop.path);
                                        if (!folderPath.empty())
                                        {
                                            UI_ContentBrowser::NavigateToPath(state, folderPath);
                                        }
                                    }
                                    ImGui::Separator();
                                }

                                if (!isDeleted)
                                {
                                    if (ImGui::MenuItem(isHidden ? "Show" : "Hide", "H"))
                                    {
                                        if (isHidden)
                                        {
                                            gHiddenProps.erase(prop.uniqueID);
                                        }
                                        else
                                        {
                                            gHiddenProps.insert(prop.uniqueID);
                                        }
                                    }
                                    if (ImGui::MenuItem("Delete", "Del"))
                                    {
                                        gDeletedProps.insert(prop.uniqueID);
                                        if (IsPropSelected(prop.uniqueID))
                                            ClearPropSelection();
                                    }
                                }
                                else
                                {
                                    if (ImGui::MenuItem("Restore"))
                                    {
                                        gDeletedProps.erase(prop.uniqueID);
                                    }
                                }
                                ImGui::EndPopup();
                            }

                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", prop.path.c_str());
                                ImGui::Text("ID: %u", prop.uniqueID);
                                if (isHidden) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Hidden (Ctrl+H to show all)");
                                if (isDeleted) ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Deleted");
                                ImGui::EndTooltip();
                            }

                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No content loaded");
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "Open Content Browser");
                ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "to load files");
            }

            ImGui::EndChild();

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
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}