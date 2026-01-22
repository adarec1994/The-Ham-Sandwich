    #include "UI_ModelTab.h"
#include "UI_Utils.h"
#include "UI_Globals.h"

#include "../Archive.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

#include <imgui.h>

extern void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax);

static bool EndsWithM3W(const std::wstring& str)
{
    if (str.length() < 3) return false;
    wchar_t last = str.back();
    wchar_t second = str[str.length() - 2];
    wchar_t third = str[str.length() - 3];
    if (third != L'.') return false;
    if (second != L'm' && second != L'M') return false;
    if (last != L'3') return false;
    return true;
}

struct MergedFolderEntry
{
    ArchivePtr arc;
    IFileSystemEntryPtr dir;
};

struct MergedFolder
{
    std::string path;
    std::vector<MergedFolderEntry> entries;
};

static std::vector<MergedFolder> gMergedM3Folders;
static std::vector<const Archive*> gMergedM3Archives;
static bool gMergedM3ListBuilt = false;

static bool AreArchivesSame(const std::vector<ArchivePtr>& arcs, const std::vector<const Archive*>& prev)
{
    if (arcs.size() != prev.size()) return false;
    for (size_t i = 0; i < arcs.size(); ++i)
        if (arcs[i].get() != prev[i]) return false;
    return true;
}

static void ScanM3FoldersRecursive(
    const ArchivePtr& arc,
    const IFileSystemEntryPtr& entry,
    std::map<std::string, std::vector<MergedFolderEntry>>& outMap)
{
    if (!entry || !entry->isDirectory()) return;

    bool foundHere = false;
    const auto& children = entry->getChildren();

    for (const auto& child : children)
    {
        if (!child) continue;
        if (!child->isDirectory())
        {
            if (EndsWithM3W(child->getEntryName()))
            {
                foundHere = true;
                break;
            }
        }
    }

    if (foundHere)
    {
        std::string dirName = wstring_to_utf8(entry->getEntryName());
        if (dirName.empty()) dirName = "/";
        outMap[dirName].push_back(MergedFolderEntry{arc, entry});
    }

    for (const auto& child : children)
        if (child && child->isDirectory())
            ScanM3FoldersRecursive(arc, child, outMap);
}

void UI_EnsureMergedM3List(const AppState& state)
{
    if (!AreArchivesSame(state.archives, gMergedM3Archives))
    {
        gMergedM3ListBuilt = false;
        gMergedM3Archives.clear();
        for (auto& a : state.archives) gMergedM3Archives.push_back(a.get());
    }

    if (gMergedM3ListBuilt) return;

    gMergedM3Folders.clear();
    std::map<std::string, std::vector<MergedFolderEntry>> folderMap;

    for (const auto& arc : state.archives)
    {
        if (!arc) continue;
        if (auto root = arc->getRoot())
            ScanM3FoldersRecursive(arc, root, folderMap);
    }

    gMergedM3Folders.reserve(folderMap.size());
    for (auto& kv : folderMap)
        gMergedM3Folders.push_back({kv.first, std::move(kv.second)});

    gMergedM3ListBuilt = true;
}

static void LoadSingleM3(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
{
    if (!arc || !fileEntry) return;

    gLoadedAreas.clear();
    state.currentArea.reset();
    gLoadedModel = nullptr;
    state.m3Render = nullptr;
    state.show_models_window = false;

    std::string name = wstring_to_utf8(fileEntry->getEntryName());

    M3ModelData data = M3Loader::LoadFromFile(arc, fileEntry);
    if (data.success)
    {
        gLoadedModel = std::make_shared<M3Render>(data, arc);
        gLoadedModel->setModelName(name);
        state.m3Render = gLoadedModel;
        state.show_models_window = true;

        glm::vec3 boundsMin(std::numeric_limits<float>::max());
        glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
        for (const auto& v : data.geometry.vertices)
        {
            boundsMin = glm::min(boundsMin, v.position);
            boundsMax = glm::max(boundsMax, v.position);
        }
        SnapCameraToModel(state, boundsMin, boundsMax);
    }
    else
    {
        gLoadedModel = nullptr;
        state.m3Render = nullptr;
        state.show_models_window = false;
    }
}

void UI_RenderModelTab(AppState& state, float& outContentWidth)
{
    ImGui::Text("MODELS");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    static char modelBuf[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##ModelSearch", "Search models...", modelBuf, 128);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 10));

    std::string modelQuery(modelBuf);

    if (ImGui::BeginChild("ModelTree", ImVec2(0, 0), false))
    {
        float calculated_width = 280.0f;
        std::string queryLower = ToLowerCopy(modelQuery);

        std::vector<MergedFolder*> visibleFolders;
        visibleFolders.reserve(gMergedM3Folders.size());

        for (auto& mf : gMergedM3Folders)
        {
            bool matchFolder = true;
            std::string pathLower = ToLowerCopy(mf.path);
            if (!queryLower.empty() && pathLower.find(queryLower) == std::string::npos)
                matchFolder = false;

            if (!matchFolder && !queryLower.empty())
            {
                bool childMatch = false;
                for (const auto& fe : mf.entries)
                {
                    for (const auto& child : fe.dir->getChildren())
                    {
                        if (!child || child->isDirectory()) continue;
                        if (!EndsWithM3W(child->getEntryName())) continue;

                        std::string cName = wstring_to_utf8(child->getEntryName());
                        if (ToLowerCopy(cName).find(queryLower) != std::string::npos)
                        {
                            childMatch = true;
                            break;
                        }
                    }
                    if (childMatch) break;
                }
                if (!childMatch) continue;
            }
            visibleFolders.push_back(&mf);
        }

        for (size_t i = 0; i < visibleFolders.size(); ++i)
{
    MergedFolder* mf = visibleFolders[i];

    float text_w = ImGui::CalcTextSize(mf->path.c_str()).x;
    float current_w = text_w + 50.0f;
    if (current_w > calculated_width) calculated_width = current_w;

    ImGui::PushID(static_cast<int>(i));

    if (ImGui::TreeNode(mf->path.c_str()))
    {
        std::vector<std::string> fileNames;
        fileNames.reserve(512);

        for (const auto& fe : mf->entries)
        {
            for (const auto& child : fe.dir->getChildren())
            {
                if (!child || child->isDirectory()) continue;
                if (!EndsWithM3W(child->getEntryName())) continue;

                std::string cName = wstring_to_utf8(child->getEntryName());

                if (!queryLower.empty())
                {
                    std::string pathLower2 = ToLowerCopy(mf->path);
                    bool folderMatched = (pathLower2.find(queryLower) != std::string::npos);
                    if (!folderMatched && ToLowerCopy(cName).find(queryLower) == std::string::npos)
                        continue;
                }

                fileNames.push_back(cName);
            }
        }

        std::sort(fileNames.begin(), fileNames.end());
        fileNames.erase(std::unique(fileNames.begin(), fileNames.end()), fileNames.end());

        ImGuiListClipper fileClipper;
        fileClipper.Begin(static_cast<int>(fileNames.size()));
        while (fileClipper.Step())
        {
            for (int j = fileClipper.DisplayStart; j < fileClipper.DisplayEnd; j++)
            {
                const std::string& fName = fileNames[j];

                float c_text_w = ImGui::CalcTextSize(fName.c_str()).x;
                float c_current_w = c_text_w + 50.0f + ImGui::GetStyle().IndentSpacing;
                if (c_current_w > calculated_width) calculated_width = c_current_w;

                if (ImGui::Selectable(fName.c_str()))
                {
                    for (const auto& fe : mf->entries)
                    {
                        bool found = false;
                        for (const auto& child : fe.dir->getChildren())
                        {
                            if (!child || child->isDirectory()) continue;
                            std::string cName = wstring_to_utf8(child->getEntryName());
                            if (cName != fName) continue;

                            if (auto fileEntry = std::dynamic_pointer_cast<FileEntry>(child))
                            {
                                LoadSingleM3(state, fe.arc, fileEntry);
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }
                }
            }
        }
        fileClipper.End();

        ImGui::TreePop();
    }

    ImGui::PopID();
}


        ImGui::EndChild();
        outContentWidth = calculated_width;
    }
}