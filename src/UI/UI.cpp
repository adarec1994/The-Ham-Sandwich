#include "UI.h"
#include "../About/about.h"
#include "../settings/Settings.h"
#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../Area/AreaRender.h"
#include "../models/UI_models.h"
#include "splashscreen.h"
#include "../tex/tex.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include <limits>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <codecvt>
#include <locale>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern void SnapCameraToLoaded(AppState& state);
extern void PushSplashButtonColors();
extern void PopSplashButtonColors();
extern bool gAreaIconLoaded;
extern unsigned int gAreaIconTexture;
extern bool gCharacterIconLoaded;
extern unsigned int gCharacterIconTexture;

std::vector<AreaFilePtr> gLoadedAreas;
std::shared_ptr<M3Render> gLoadedModel = nullptr;

static AreaChunkRenderPtr gSelectedChunk = nullptr;
static int gSelectedChunkIndex = -1;
static int gSelectedAreaIndex = -1;
static std::string gSelectedChunkAreaName;

static std::string wstring_to_utf8(const std::wstring& str)
{
    if (str.empty()) return {};
#ifdef _WIN32s
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0, NULL, NULL);
    std::string out(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), out.data(), size_needed, NULL, NULL);
    return out;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(str);
#endif
}

static bool EndsWithNoCase(const std::string& s, const std::string& suffix)
{
    if (s.size() < suffix.size())
        return false;
    const size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        const auto a = static_cast<unsigned char>(
            std::tolower(static_cast<unsigned char>(s[off + i]))
        );
        const auto b = static_cast<unsigned char>(
            std::tolower(static_cast<unsigned char>(suffix[i]))
        );
        if (a != b)
            return false;
    }
    return true;
}

static std::string ToLowerCopy(std::string s)
{
    for (auto& c : s)
    {
        c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(c))
            );
    }
    return s;
}

static bool EndsWithM3W(const std::wstring& str) {
    if (str.length() < 3) return false;
    wchar_t last = str.back();
    wchar_t second = str[str.length() - 2];
    wchar_t third = str[str.length() - 3];

    if (third != L'.') return false;
    if (second != L'm' && second != L'M') return false;
    if (last != L'3') return false;
    return true;
}

static std::string GetExtLower(const std::string& name)
{
    std::filesystem::path p(name);
    std::string e = p.extension().string();
    if (e.empty()) return "";
    return ToLowerCopy(e);
}

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

static bool RayIntersectsBox(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxMin, glm::vec3 boxMax, float& t)
{
    glm::vec3 tMin = (boxMin - rayOrigin) / rayDir;
    glm::vec3 tMax = (boxMax - rayOrigin) / rayDir;
    glm::vec3 t1 = glm::min(tMin, tMax);
    glm::vec3 t2 = glm::max(tMin, tMax);
    float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);

    if (tNear > tFar || tFar < 0.0f) return false;
    t = tNear;
    return true;
}

void CheckChunkSelection(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &display_w, &display_h);

    if (display_w <= 0 || display_h <= 0)
        return;

    const glm::mat4 view = glm::lookAt(
        state.camera.Position,
        state.camera.Position + state.camera.Front,
        state.camera.Up
    );

    const glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(display_w) / static_cast<float>(display_h),
        0.1f,
        20000.0f
    );

    const float mouseX = io.MousePos.x;
    const float mouseY = static_cast<float>(display_h) - io.MousePos.y;

    const glm::vec3 rayStart = glm::unProject(
        glm::vec3(mouseX, mouseY, 0.0f),
        view,
        proj,
        glm::vec4(
            0.0f,
            0.0f,
            static_cast<float>(display_w),
            static_cast<float>(display_h)
        )
    );

    const glm::vec3 rayEnd = glm::unProject(
        glm::vec3(mouseX, mouseY, 1.0f),
        view,
        proj,
        glm::vec4(
            0.0f,
            0.0f,
            static_cast<float>(display_w),
            static_cast<float>(display_h)
        )
    );

    const glm::vec3 rayDir = glm::normalize(rayEnd - rayStart);

    float closestDist = std::numeric_limits<float>::max();
    AreaChunkRenderPtr hitChunk = nullptr;
    int hitIndex = -1;
    int hitAreaIdx = -1;
    std::string hitAreaName;

    for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
    {
        const auto& area = gLoadedAreas[areaIdx];
        if (!area)
            continue;

        const glm::vec3 center = (area->getMinBounds() + area->getMaxBounds()) * 0.5f;
        const float rot = area->getRotation();

        glm::mat4 model(1.0f);
        model = glm::translate(model, center);
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, -center);

        const glm::mat4 invModel = glm::inverse(model);

        const glm::vec3 rayStartLocal =
            glm::vec3(invModel * glm::vec4(rayStart, 1.0f));

        const glm::vec3 rayDirLocal =
            glm::vec3(invModel * glm::vec4(rayDir, 0.0f));

        const auto& chunks = area->getChunks();
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            const auto& chunk = chunks[i];
            if (!chunk)
                continue;

            const glm::vec3 minB = chunk->getMinBounds();
            const glm::vec3 maxB = chunk->getMaxBounds();
            if (minB.x > maxB.x)
                continue;

            float dist = 0.0f;
            if (RayIntersectsBox(rayStartLocal, rayDirLocal, minB, maxB, dist))
            {
                if (dist < closestDist)
                {
                    closestDist = dist;
                    hitChunk = chunk;
                    hitIndex = static_cast<int>(i);
                    hitAreaIdx = static_cast<int>(areaIdx);
                    hitAreaName = "Area " + std::to_string(areaIdx);
                }
            }
        }
    }

    if (hitChunk)
    {
        gSelectedChunk = hitChunk;
        gSelectedChunkIndex = hitIndex;
        gSelectedAreaIndex = hitAreaIdx;
        gSelectedChunkAreaName = hitAreaName;
    }
}

static void LoadAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry)
{
    if (!arc || !folderEntry || !folderEntry->isDirectory())
        return;

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;

    for (const auto& child : folderEntry->getChildren())
    {
        if (!child || child->isDirectory())
            continue;

        const std::string childName = wstring_to_utf8(child->getEntryName());
        if (!EndsWithNoCase(childName, ".area"))
            continue;

        const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(child);
        if (!fileEntry)
            continue;

        std::cout << "Loading Area: " << childName << std::endl;

        const auto af = std::make_shared<AreaFile>(arc, fileEntry);
        if (af->load())
        {
            std::cout << "Area Loaded Successfully." << std::endl;
            gLoadedAreas.push_back(af);
        }
        else
        {
            std::cout << "Failed to load Area." << std::endl;
        }
    }

    if (!gLoadedAreas.empty())
    {
        state.currentArea = gLoadedAreas.back();
        SnapCameraToLoaded(state);
    }
    else
    {
        state.currentArea.reset();
    }
}

static void LoadSingleArea(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
{
    if (!arc || !fileEntry) return;

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
    gLoadedModel = nullptr;

    std::string name = wstring_to_utf8(fileEntry->getEntryName());
    std::cout << "Loading Area: " << name << std::endl;

    auto af = std::make_shared<AreaFile>(arc, fileEntry);
    if (af->load())
    {
        std::cout << "Area Loaded Successfully." << std::endl;
        gLoadedAreas.push_back(af);
        state.currentArea = af;
        SnapCameraToLoaded(state);
    }
    else
    {
        std::cout << "Failed to load Area." << std::endl;
        state.currentArea.reset();
    }
}

static void LoadSingleM3(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
{
    if (!arc || !fileEntry) return;

    gLoadedAreas.clear();
    state.currentArea.reset();

    state.m3Render = nullptr;
    state.show_models_window = false;

    std::string name = wstring_to_utf8(fileEntry->getEntryName());
    std::cout << "Loading M3 Model: " << name << std::endl;

    M3ModelData data = M3Loader::LoadFromFile(arc, fileEntry);
    if (data.success)
    {
        state.m3Render = std::make_shared<M3Render>(data, arc);
        state.show_models_window = true;

        std::cout << "M3 Loaded. Vertices: " << data.vertices.size() << ", Indices: " << data.indices.size() << std::endl;

        state.camera.Position = glm::vec3(0, 1.0f, 3.0f);
        state.camera.Front = glm::vec3(0, 0, -1.0f);
        state.camera.Up = glm::vec3(0, 1.0f, 0);
    }
    else
    {
        std::cout << "Failed to parse M3 Model." << std::endl;
        state.m3Render = nullptr;
        state.show_models_window = false;
    }
}

void RenderAreas(const AppState& state, int display_w, int display_h)
{
    static int frameCount = 0;
    if (frameCount < 5)
    {
        std::cout << "RenderAreas called, frame " << frameCount
                  << ", areas=" << gLoadedAreas.size() << "\n";
        frameCount++;
    }

    if (display_w <= 0 || display_h <= 0)
        return;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);

    const glm::mat4 view = glm::lookAt(
        state.camera.Position,
        state.camera.Position + state.camera.Front,
        state.camera.Up
    );

    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(display_w) / static_cast<float>(display_h),
        0.1f,
        100000.0f
    );

    if (gLoadedModel)
    {
        gLoadedModel->render(view, projection);
    }
    else if (!gLoadedAreas.empty() && state.areaRender)
    {
        const uint32_t prog = state.areaRender->getProgram();
        if (prog != 0)
        {
            glUseProgram(prog);

            const GLint viewLoc = glGetUniformLocation(prog, "view");
            const GLint projLoc = glGetUniformLocation(prog, "projection");

            if (viewLoc != -1)
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            if (projLoc != -1)
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

            for (const auto& area : gLoadedAreas)
            {
                if (area)
                    area->render(view, projection, prog, gSelectedChunk);
            }
        }
    }
}

static void RenderEntryRecursive_Impl(
    AppState& state,
    const IFileSystemEntryPtr& entry,
    const IFileSystemEntryPtr& parentDir,
    const ArchivePtr& currentArc,
    float& max_width,
    float depth)
{
    if (entry->isDirectory())
    {
        std::string name = wstring_to_utf8(entry->getEntryName());
        if (name.empty()) name = "/";

        const float indent_px = depth * ImGui::GetStyle().IndentSpacing;
        const float text_w = ImGui::CalcTextSize(name.c_str()).x;
        const float current_w = indent_px + text_w + 50.0f;
        if (current_w > max_width) max_width = current_w;

        if (const bool open = ImGui::TreeNode(name.c_str()); open)
        {
            const auto& children = entry->getChildren();

            std::vector<IFileSystemEntryPtr> folders;
            std::vector<IFileSystemEntryPtr> files;
            folders.reserve(children.size());
            files.reserve(children.size());

            for (const auto& child : children)
            {
                if (child->isDirectory()) folders.push_back(child);
                else files.push_back(child);
            }

            for (const auto& folder : folders)
            {
                RenderEntryRecursive_Impl(state, folder, entry, currentArc, max_width, depth + 1.0f);
            }

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(files.size()));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    const auto& f = files[static_cast<size_t>(i)];
                    const std::string fname = wstring_to_utf8(f->getEntryName());

                    const float f_indent_px = (depth + 1.0f) * ImGui::GetStyle().IndentSpacing;
                    const float f_text_w = ImGui::CalcTextSize(fname.c_str()).x;
                    const float f_current_w = f_indent_px + f_text_w + 50.0f;
                    if (f_current_w > max_width) max_width = f_current_w;

                    if (ImGui::Selectable(fname.c_str()))
                    {
                        if (EndsWithNoCase(fname, ".area"))
                        {
                            if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f); fileEntry && currentArc)
                                LoadSingleArea(state, currentArc, fileEntry);
                        }
                        else if (EndsWithNoCase(fname, ".tex"))
                        {
                            if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f); fileEntry && currentArc)
                                Tex::OpenTexPreviewFromEntry(state, currentArc, fileEntry);
                        }
                        else if (EndsWithNoCase(fname, ".m3"))
                        {
                            if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f); fileEntry && currentArc)
                                LoadSingleM3(state, currentArc, fileEntry);
                        }
                    }
                }
            }

            ImGui::TreePop();
        }
    }
    else
    {
        const std::string name = wstring_to_utf8(entry->getEntryName());
        const float indent_px = depth * ImGui::GetStyle().IndentSpacing;
        const float text_w = ImGui::CalcTextSize(name.c_str()).x;
        const float current_w = indent_px + text_w + 50.0f;
        if (current_w > max_width) max_width = current_w;

        if (ImGui::Selectable(name.c_str()))
        {
            if (EndsWithNoCase(name, ".area"))
            {
                if (currentArc && parentDir && parentDir->isDirectory())
                {
                    LoadAreasInFolder(state, currentArc, parentDir);
                }
                else
                {
                    if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry); fileEntry && currentArc)
                        LoadSingleArea(state, currentArc, fileEntry);
                }
            }
            else if (EndsWithNoCase(name, ".tex"))
            {
                if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry); fileEntry && currentArc)
                    Tex::OpenTexPreviewFromEntry(state, currentArc, fileEntry);
            }
            else if (EndsWithNoCase(name, ".m3"))
            {
                if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry); fileEntry && currentArc)
                    LoadSingleM3(state, currentArc, fileEntry);
            }
        }
    }
}

void RenderEntryRecursive(AppState& state, const IFileSystemEntryPtr& entry, const ArchivePtr& currentArc, float& max_width, float depth)
{
    RenderEntryRecursive_Impl(state, entry, nullptr, currentArc, max_width, depth);
}

struct AreaFilterCache
{
    std::string queryLower;
    std::vector<const Archive*> archives;
    std::unordered_map<const IFileSystemEntry*, bool> subtreeHas;
    std::unordered_map<const IFileSystemEntry*, bool> dirHas;
};

static AreaFilterCache gAreaCache;

struct MergedFolder {
    std::string path;
    std::vector<IFileSystemEntryPtr> entries;
};

std::vector<MergedFolder> gMergedM3Folders;
std::vector<const Archive*> gMergedM3Archives;
bool gMergedM3ListBuilt = false;

static bool ContainsLowerFast(const std::string& hayLower, const std::string& needleLower)
{
    if (needleLower.empty()) return true;
    return hayLower.find(needleLower) != std::string::npos;
}

static bool BuildAreaCacheForDir(const IFileSystemEntryPtr& entry, const std::string& queryLower)
{
    if (!entry || !entry->isDirectory())
        return false;

    const IFileSystemEntry* key = entry.get();
    auto it = gAreaCache.subtreeHas.find(key);
    if (it != gAreaCache.subtreeHas.end())
        return it->second;

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
    if (!entry || !entry->isDirectory())
        return false;

    const IFileSystemEntry* key = entry.get();
    auto it = gAreaCache.subtreeHas.find(key);
    if (it != gAreaCache.subtreeHas.end())
        return it->second;

    std::string ql = ToLowerCopy(query);
    return BuildAreaCacheForDir(entry, ql);
}

static void RenderAreaTreeFiltered(AppState& state,
                                  const IFileSystemEntryPtr& entry,
                                  const IFileSystemEntryPtr& parentDir,
                                  const ArchivePtr& arc,
                                  const std::string& query,
                                  float& max_width,
                                  float depth)
{
    if (!entry || !entry->isDirectory())
        return;

    const IFileSystemEntry* key = entry.get();

    auto itSub = gAreaCache.subtreeHas.find(key);
    if (itSub != gAreaCache.subtreeHas.end() && !itSub->second)
        return;

    if (!HasAreaInSubtree(entry, query))
        return;

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
            RenderAreaTreeFiltered(state, child, entry, arc, query, max_width, depth);
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
        if (ImGui::Button(loadLabel.c_str()))
            LoadAreasInFolder(state, arc, entry);

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
        {
            RenderAreaTreeFiltered(state, child, entry, arc, query, max_width, depth + 1.0f);
        }

        ImGui::PopID();
        ImGui::TreePop();
    }
}

static void ScanM3FoldersRecursive(const IFileSystemEntryPtr& entry, std::map<std::string, std::vector<IFileSystemEntryPtr>>& outMap){
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
        outMap[dirName].push_back(entry);
    }

    for (const auto& child : children)
    {
        if (child && child->isDirectory())
        {
            ScanM3FoldersRecursive(child, outMap);
        }
    }
}

static void EnsureMergedM3List(const AppState& state){
    if (!AreArchivesSame(state.archives, gMergedM3Archives))
    {
        gMergedM3ListBuilt = false;
        gMergedM3Archives.clear();
        for (auto& a : state.archives) gMergedM3Archives.push_back(a.get());
    }

    if (gMergedM3ListBuilt) return;

    gMergedM3Folders.clear();
    std::map<std::string, std::vector<IFileSystemEntryPtr>> folderMap;

    for (const auto& arc : state.archives)
    {
        if (auto root = arc->getRoot())
        {
            ScanM3FoldersRecursive(root, folderMap);
        }
    }

    gMergedM3Folders.reserve(folderMap.size());
    for (auto& kv : folderMap)
    {
        gMergedM3Folders.push_back({kv.first, std::move(kv.second)});
    }

    gMergedM3ListBuilt = true;
}

void RenderUI(AppState& state)
{
    if (!state.archivesLoaded)
    {
        PushSplashButtonColors();
        RenderSplashScreen(state);
        PopSplashButtonColors();
        return;
    }

    EnsureMergedM3List(state);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiIO& io = ImGui::GetIO();

    float target_panel_width = state.sidebar_visible ? state.contentWidth : 0.0f;
    if (state.sidebar_visible && target_panel_width < 280.0f) target_panel_width = 280.0f;
    if (target_panel_width > viewport->Size.x * 0.5f) target_panel_width = viewport->Size.x * 0.5f;

    float slide_speed = 1800.0f;
    float step = slide_speed * io.DeltaTime;

    if (state.sidebar_current_width < target_panel_width)
    {
        state.sidebar_current_width += step;
        if (state.sidebar_current_width > target_panel_width) state.sidebar_current_width = target_panel_width;
    }
    else if (state.sidebar_current_width > target_panel_width)
    {
        state.sidebar_current_width -= step;
        if (state.sidebar_current_width < target_panel_width) state.sidebar_current_width = target_panel_width;
    }

    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("SpeedOverlay", nullptr, overlay_flags))
    {
        ImGui::Text("Camera Speed: %.1f", state.camera.MovementSpeed);
    }
    ImGui::End();

    if (gSelectedChunk)
    {
        ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 250.0f, 80.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240, 0));

        if (ImGui::Begin("Chunk Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Index: %d", gSelectedChunkIndex);
            if (!gSelectedChunkAreaName.empty())
                ImGui::Text("%s", gSelectedChunkAreaName.c_str());

            ImGui::Separator();

            glm::vec3 minB = gSelectedChunk->getMinBounds();
            glm::vec3 maxB = gSelectedChunk->getMaxBounds();

            ImGui::Text("Bounds Min: (%.1f, %.1f, %.1f)", minB.x, minB.y, minB.z);
            ImGui::Text("Bounds Max: (%.1f, %.1f, %.1f)", maxB.x, maxB.y, maxB.z);

            ImGui::Separator();
            ImGui::Text("Flags: 0x%X", gSelectedChunk->getFlags());
            ImGui::Text("Avg Height: %.2f", gSelectedChunk->getAverageHeight());
            ImGui::Text("Max Height: %.2f", gSelectedChunk->getMaxHeight());

            ImGui::Separator();
            if (gSelectedAreaIndex >= 0 && gSelectedAreaIndex <
                static_cast<int>(gLoadedAreas.size())) {
                if (ImGui::Button("Rotate Area 90")) {
                    gLoadedAreas[gSelectedAreaIndex]->rotate90();
                }
            }
        }
        ImGui::End();
    }

    float strip_width = 70.0f;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(strip_width, viewport->Size.y));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags strip_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##Strip", nullptr, strip_flags))
    {
        float button_height = 50.0f;

        if (bool is_active = (state.sidebar_visible && state.active_tab_index == 0); is_active)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(viewport->Pos.x, ImGui::GetCursorScreenPos().y + (button_height * 0.1f)),
                ImVec2(viewport->Pos.x + 3, ImGui::GetCursorScreenPos().y + (button_height * 0.9f)),
                IM_COL32(100, 149, 237, 255)
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

        if (state.iconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##FileTab", reinterpret_cast<void*>(static_cast<intptr_t>(state.iconTexture)), ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                if (state.active_tab_index == 0) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 0; state.sidebar_visible = true; }
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Files", ImVec2(strip_width, button_height)))
            {
                if (state.active_tab_index == 0) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 0; state.sidebar_visible = true; }
            }
        }

        ImGui::PopStyleColor();

        if (bool is_area_active = (state.sidebar_visible && state.active_tab_index == 1); is_area_active)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(viewport->Pos.x, ImGui::GetCursorScreenPos().y + (button_height * 0.1f)),
                ImVec2(viewport->Pos.x + 3, ImGui::GetCursorScreenPos().y + (button_height * 0.9f)),
                IM_COL32(100, 149, 237, 255)
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

        if (gAreaIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##AreaTab", reinterpret_cast<void*>(static_cast<intptr_t>(gAreaIconTexture)), ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                if (state.active_tab_index == 1) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 1; state.sidebar_visible = true; }
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Area", ImVec2(strip_width, button_height)))
            {
                if (state.active_tab_index == 1) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 1; state.sidebar_visible = true; }
            }
        }

        ImGui::PopStyleColor();

        if (bool is_model_active = (state.sidebar_visible && state.active_tab_index == 2); is_model_active)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(viewport->Pos.x, ImGui::GetCursorScreenPos().y + (button_height * 0.1f)),
                ImVec2(viewport->Pos.x + 3, ImGui::GetCursorScreenPos().y + (button_height * 0.9f)),
                IM_COL32(100, 149, 237, 255)
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

        if (gCharacterIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##ModelTab", reinterpret_cast<void*>(static_cast<intptr_t>(gCharacterIconTexture)), ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                if (state.active_tab_index == 2) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 2; state.sidebar_visible = true; }
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Models", ImVec2(strip_width, button_height)))
            {
                if (state.active_tab_index == 2) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 2; state.sidebar_visible = true; }
            }
        }

        ImGui::PopStyleColor();

        float bottom_margin = 10.0f;
        float settings_y = ImGui::GetWindowHeight() - button_height - bottom_margin;
        float about_y = settings_y - button_height;

        ImGui::SetCursorPosY(about_y);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (state.aboutIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##AboutTab", reinterpret_cast<void*>(static_cast<intptr_t>(state.aboutIconTexture)), ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                state.show_about_window = !state.show_about_window;
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("About", ImVec2(strip_width, button_height)))
                state.show_about_window = !state.show_about_window;
        }
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(settings_y);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (state.settingsIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##SettingsTab", reinterpret_cast<void*>(static_cast<intptr_t>(state.settingsIconTexture)), ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                state.show_settings_window = !state.show_settings_window;
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Set", ImVec2(strip_width, button_height)))
                state.show_settings_window = !state.show_settings_window;
        }
        ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    if (state.sidebar_current_width > 1.0f)
    {
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + strip_width, viewport->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(state.sidebar_current_width, viewport->Size.y));
        ImGuiWindowFlags panel_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("##Panel", nullptr, panel_flags))
        {
            ImGui::Spacing();
            if (state.active_tab_index == 0)
            {
                ImGui::Text("EXPLORER");
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                char buf[64] = "";
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##Search", "Search files...", buf, 64);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 10));

                if (ImGui::BeginChild("FileTree", ImVec2(0, 0), false))
                {
                    float calculated_width = 280.0f;

                    for (auto& archive : state.archives)
                    {
                        std::filesystem::path p(archive->getPath());
                        std::string archiveName = p.filename().string();

                        float header_w = ImGui::CalcTextSize(archiveName.c_str()).x + 30.0f;
                        if (header_w > calculated_width) calculated_width = header_w;

                        if (ImGui::TreeNode(archiveName.c_str()))
                        {
                            if (auto root = archive->getRoot())
                            {
                                for (const auto& child : root->getChildren())
                                    RenderEntryRecursive_Impl(state, child, root, archive, calculated_width, 1.0f);
                            }
                            ImGui::TreePop();
                        }
                    }

                    ImGui::EndChild();
                    state.contentWidth = calculated_width;
                }
            }
            else if (state.active_tab_index == 1)
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
                            if (it != gAreaCache.subtreeHas.end())
                            {
                                hasMatch = it->second;
                            }
                            else
                            {
                                hasMatch = HasAreaInSubtree(mapEntry, areaQuery);
                            }

                            if (hasMatch)
                            {
                                RenderAreaTreeFiltered(state, mapEntry, root, archive, areaQuery, calculated_width, 0.0f);
                            }
                        }
                    }

                    ImGui::EndChild();
                    state.contentWidth = calculated_width;
                }
            }
            else if (state.active_tab_index == 2)
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
                        {
                            matchFolder = false;
                        }

                        if (!matchFolder && !queryLower.empty())
                        {
                            bool childMatch = false;
                            for (const auto& entry : mf.entries)
                            {
                                for (const auto& child : entry->getChildren())
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

                    ImGuiListClipper folderClipper;
                    folderClipper.Begin(static_cast<int>(visibleFolders.size()));

                    while (folderClipper.Step())
                    {
                        for (int i = folderClipper.DisplayStart; i < folderClipper.DisplayEnd; i++)
                        {
                            MergedFolder* mf = visibleFolders[i];
                            float text_w = ImGui::CalcTextSize(mf->path.c_str()).x;
                            float current_w = text_w + 50.0f;
                            if (current_w > calculated_width) calculated_width = current_w;

                            if (ImGui::TreeNode(mf->path.c_str()))
                            {
                                std::vector<std::string> fileNames;
                                for (const auto& entry : mf->entries)
                                {
                                    for (const auto& child : entry->getChildren())
                                    {
                                        if (!child || child->isDirectory()) continue;

                                        if (!EndsWithM3W(child->getEntryName())) continue;

                                        std::string cName = wstring_to_utf8(child->getEntryName());

                                        if (!queryLower.empty())
                                        {
                                            std::string pathLower = ToLowerCopy(mf->path);
                                            bool folderMatched = (pathLower.find(queryLower) != std::string::npos);
                                            if (!folderMatched && ToLowerCopy(cName).find(queryLower) == std::string::npos)
                                                continue;
                                        }
                                        fileNames.push_back(cName);
                                    }
                                }
                                std::sort(fileNames.begin(), fileNames.end());
                                auto last = std::unique(fileNames.begin(), fileNames.end());
                                fileNames.erase(last, fileNames.end());

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
                                            for (const auto& entry : mf->entries)
                                            {
                                                bool found = false;
                                                for (const auto& child : entry->getChildren())
                                                {
                                                    if (!child || child->isDirectory()) continue;
                                                    std::string cName = wstring_to_utf8(child->getEntryName());
                                                    if (cName == fName)
                                                    {
                                                        if (auto fileEntry = std::dynamic_pointer_cast<FileEntry>(child))
                                                        {
                                                            ArchivePtr validArc = nullptr;
                                                            if (!state.archives.empty()) validArc = state.archives[0];

                                                            LoadSingleM3(state, validArc, fileEntry);
                                                            found = true;
                                                            break;
                                                        }
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
                        }
                    }
                    folderClipper.End();

                    ImGui::EndChild();
                    state.contentWidth = calculated_width;
                }
            }
        }
        ImGui::End();
    }

    RenderSettingsWindow(&state.show_settings_window);
    RenderAboutWindow(&state.show_about_window);

    if (state.texPreview)
    {
        Tex::RenderTexPreviewWindow(*state.texPreview);
    }

    if (state.show_models_window && state.m3Render)
    {
        UI_Models::Draw(state);
    }

}