#include "UI_AreaTab.h"
#include "UI_Globals.h"
#include "UI_Utils.h"

#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../tex/tex.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

#include <imgui.h>

extern void SnapCameraToLoaded(AppState& state);

struct AreaFilterCache
{
    std::string queryLower;
    std::vector<const Archive*> archives;
    std::unordered_map<const IFileSystemEntry*, bool> subtreeHas;
    std::unordered_map<const IFileSystemEntry*, bool> dirHas;
};

static AreaFilterCache gAreaCache;

static std::string FormatFolderLabel(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 2);
    char prev = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (c == '_' || c == '-') c = ' ';
        bool isUpper = (c >= 'A' && c <= 'Z');
        bool prevLower = (prev >= 'a' && prev <= 'z');
        bool prevDigit = (prev >= '0' && prev <= '9');
        bool currDigit = (c >= '0' && c <= '9');

        if (i > 0 && c != ' ' && isUpper && (prevLower || prevDigit))
            out.push_back(' ');
        if (i > 0 && currDigit && !prevDigit && prev != ' ')
            out.push_back(' ');

        out.push_back(c);
        prev = c;
    }
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

static bool BuildAreaCacheForDir(const IFileSystemEntryPtr& entry, const std::string& queryLower)
{
    if (!entry || !entry->isDirectory()) return false;

    const IFileSystemEntry* key = entry.get();
    auto it = gAreaCache.subtreeHas.find(key);
    if (it != gAreaCache.subtreeHas.end()) return it->second;

    bool dirHasMatch = false;
    bool anyChildMatch = false;

    for (auto& child : entry->getChildren())
    {
        if (!child) continue;
        if (!child->isDirectory())
        {
            std::string n = wstring_to_utf8(child->getEntryName());
            if (!EndsWithNoCase(n, ".area")) continue;

            if (queryLower.empty())
            {
                dirHasMatch = true;
            }
            else
            {
                std::string nl = ToLowerCopy(n);
                if (ContainsLowerFast(nl, queryLower))
                    dirHasMatch = true;
            }
        }
    }

    for (auto& child : entry->getChildren())
    {
        if (!child || !child->isDirectory()) continue;
        if (BuildAreaCacheForDir(child, queryLower))
            anyChildMatch = true;
    }

    bool has = dirHasMatch || anyChildMatch;
    gAreaCache.dirHas[key] = dirHasMatch;
    gAreaCache.subtreeHas[key] = has;
    return has;
}

static bool AreArchivesSame(const std::vector<ArchivePtr>& arcs, const std::vector<const Archive*>& prev)
{
    if (arcs.size() != prev.size()) return false;
    for (size_t i = 0; i < arcs.size(); ++i)
        if (arcs[i].get() != prev[i]) return false;
    return true;
}

static void EnsureAreaCache(const AppState& state, const std::string& query)
{
    std::string ql = ToLowerCopy(query);

    bool needRebuild = false;
    if (ql != gAreaCache.queryLower) needRebuild = true;
    if (!AreArchivesSame(state.archives, gAreaCache.archives)) needRebuild = true;

    if (!needRebuild) return;

    gAreaCache.queryLower = ql;
    gAreaCache.archives.clear();
    gAreaCache.archives.reserve(state.archives.size());
    for (auto& a : state.archives) gAreaCache.archives.push_back(a.get());

    gAreaCache.subtreeHas.clear();
    gAreaCache.dirHas.clear();

    for (auto& archive : state.archives)
    {
        auto root = archive ? archive->getRoot() : nullptr;
        if (!root) continue;

        for (auto& child : root->getChildren())
        {
            if (child && child->isDirectory())
                BuildAreaCacheForDir(child, gAreaCache.queryLower);
        }
    }
}

static bool DirHasAreaFiles(const IFileSystemEntryPtr& dir, const std::string& query)
{
    if (!dir || !dir->isDirectory()) return false;
    const IFileSystemEntry* key = dir.get();
    auto it = gAreaCache.dirHas.find(key);
    if (it != gAreaCache.dirHas.end()) return it->second;

    std::string ql = ToLowerCopy(query);

    bool dirHasMatch = false;
    for (auto& child : dir->getChildren())
    {
        if (!child || child->isDirectory()) continue;
        std::string n = wstring_to_utf8(child->getEntryName());
        if (!EndsWithNoCase(n, ".area")) continue;
        if (ql.empty())
        {
            dirHasMatch = true;
            break;
        }
        std::string nl = ToLowerCopy(n);
        if (ContainsLowerFast(nl, ql))
        {
            dirHasMatch = true;
            break;
        }
    }
    gAreaCache.dirHas[key] = dirHasMatch;
    return dirHasMatch;
}

static bool HasAreaInSubtree(const IFileSystemEntryPtr& entry, const std::string& query)
{
    if (!entry || !entry->isDirectory()) return false;

    const IFileSystemEntry* key = entry.get();
    auto it = gAreaCache.subtreeHas.find(key);
    if (it != gAreaCache.subtreeHas.end()) return it->second;

    std::string ql = ToLowerCopy(query);
    return BuildAreaCacheForDir(entry, ql);
}

static void LoadSingleArea(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
{
    if (!arc || !fileEntry) return;

    ResetAreaReferencePosition();

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
    gSelectedChunkIndex = -1;
    gSelectedAreaIndex = -1;
    gSelectedAreaName.clear();

    auto af = std::make_shared<AreaFile>(arc, fileEntry);
    if (af->load())
    {
        gLoadedAreas.push_back(af);
        state.currentArea = af;
        SnapCameraToLoaded(state);
    }
    else
    {
        state.currentArea.reset();
    }
}

static void RenderAreaTreeFiltered(AppState& state,
                                  const IFileSystemEntryPtr& entry,
                                  const ArchivePtr& arc,
                                  const std::string& query,
                                  float& max_width,
                                  float depth)
{
    if (!entry || !entry->isDirectory()) return;

    const IFileSystemEntry* key = entry.get();
    auto itSub = gAreaCache.subtreeHas.find(key);
    if (itSub != gAreaCache.subtreeHas.end() && !itSub->second) return;

    if (!HasAreaInSubtree(entry, query)) return;

    std::vector<IFileSystemEntryPtr> childDirs;
    std::vector<IFileSystemEntryPtr> childFiles;
    childDirs.reserve(entry->getChildren().size());
    childFiles.reserve(entry->getChildren().size());

    bool hasQuery = !query.empty();
    std::string queryLower = hasQuery ? ToLowerCopy(query) : "";

    for (auto& child : entry->getChildren())
    {
        if (!child) continue;
        if (child->isDirectory())
        {
            const IFileSystemEntry* ck = child.get();
            auto it = gAreaCache.subtreeHas.find(ck);
            if ((it != gAreaCache.subtreeHas.end()) ? it->second : HasAreaInSubtree(child, query))
                childDirs.push_back(child);
        }
        else
        {
            if (hasQuery)
            {
                std::string name = wstring_to_utf8(child->getEntryName());
                if (!ContainsLowerFast(ToLowerCopy(name), queryLower)) continue;
            }
            childFiles.push_back(child);
        }
    }

    bool showNode = DirHasAreaFiles(entry, query);

    if (!showNode)
    {
        for (auto& child : childDirs)
            RenderAreaTreeFiltered(state, child, arc, query, max_width, depth);
        return;
    }

    std::string name = wstring_to_utf8(entry->getEntryName());
    if (name.empty()) name = "/";

    float indent_px = depth * ImGui::GetStyle().IndentSpacing;
    float text_w = ImGui::CalcTextSize(name.c_str()).x;
    float current_w = indent_px + text_w + 50.0f;
    if (current_w > max_width) max_width = current_w;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx(name.c_str(), flags))
    {
        ImGui::PushID(static_cast<const void*>(entry.get()));

        std::string loadLabel = std::string("Load ") + FormatFolderLabel(name);

        if (gIsLoadingAreas)
        {
            ImGui::BeginDisabled();
            ImGui::Button(loadLabel.c_str());
            ImGui::EndDisabled();
        }
        else if (ImGui::Button(loadLabel.c_str()))
        {
            StartLoadingAreasInFolder(state, arc, entry);
        }

        if (!childFiles.empty())
        {
            std::sort(childFiles.begin(), childFiles.end(), [](const IFileSystemEntryPtr& A, const IFileSystemEntryPtr& B) {
                return A->getEntryName() < B->getEntryName();
            });

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::Separator();
            ImGui::Text("Files (%zu)", childFiles.size());
            ImGui::Separator();

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(childFiles.size()));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    auto& f = childFiles[i];
                    std::string fname = wstring_to_utf8(f->getEntryName());

                    if (ImGui::Selectable(fname.c_str()))
                    {
                        std::string fExt = GetExtLower(fname);
                        if (fExt == ".area")
                        {
                            if (auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f))
                                LoadSingleArea(state, arc, fileEntry);
                        }
                        else if (fExt == ".tex")
                        {
                            if (auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f))
                                Tex::OpenTexPreviewFromEntry(state, arc, fileEntry);
                        }
                    }
                }
            }
        }

        for (auto& child : childDirs)
            RenderAreaTreeFiltered(state, child, arc, query, max_width, depth + 1.0f);

        ImGui::PopID();
        ImGui::TreePop();
    }
}

void UI_RenderAreaTab(AppState& state, float& outContentWidth)
{
    ImGui::Text("AREA");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    static char areaBuf[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##AreaSearch", "Search areas...", areaBuf, 128);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 10));

    std::string areaQuery(areaBuf);
    EnsureAreaCache(state, areaQuery);

    if (ImGui::BeginChild("AreaTree", ImVec2(0, 0), false))
    {
        float calculated_width = 280.0f;

        for (auto& archive : state.archives)
        {
            if (auto root = archive->getRoot())
            {
                IFileSystemEntryPtr mapEntry = nullptr;
                for (auto& child : root->getChildren())
                {
                    if (!child || !child->isDirectory()) continue;
                    std::string cname = wstring_to_utf8(child->getEntryName());
                    if (ToLowerCopy(cname) == "map")
                    {
                        mapEntry = child;
                        break;
                    }
                }
                if (!mapEntry) continue;

                const IFileSystemEntry* ck = mapEntry.get();
                bool hasMatch = false;
                auto it = gAreaCache.subtreeHas.find(ck);
                if (it != gAreaCache.subtreeHas.end()) hasMatch = it->second;
                else hasMatch = HasAreaInSubtree(mapEntry, areaQuery);

                if (hasMatch)
                    RenderAreaTreeFiltered(state, mapEntry, archive, areaQuery, calculated_width, 0.0f);
            }
        }

        ImGui::EndChild();
        outContentWidth = calculated_width;
    }
}