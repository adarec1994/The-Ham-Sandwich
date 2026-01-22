#define IMGUI_DEFINE_MATH_OPERATORS
#include "UI_ContentBrowser.h"
#include "UI_Globals.h"
#include "UI_Utils.h"
#include "UI_Tables.h"
#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include "../tex/tex.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <glad/glad.h>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <condition_variable>

extern void SnapCameraToLoaded(AppState& state);
extern void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax);

namespace UI_ContentBrowser {
    static bool sIsOpen = false;
    static bool sIsDocked = false;
    static float sCurrentHeight = 0.0f;
    static float sTargetHeight = 300.0f;
    static float sAnimSpeed = 1500.0f;
    static float sTreeWidth = 200.0f;
    static ImTextureID sFolderIcon = 0;
    static ImTextureID sContentBrowserIcon = 0;
    static ImTextureID sTblIcon = 0;
    static ImTextureID sAreaIcon = 0;
    static ImTextureID sAudioIcon = 0;

    static IFileSystemEntryPtr sSelectedFolder = nullptr;
    static ArchivePtr sSelectedArchive = nullptr;
    static std::string sSelectedPath;

    static char sSearchFilter[256] = "";

    struct FileInfo
    {
        std::string name;
        std::string extension;
        IFileSystemEntryPtr entry;
        ArchivePtr archive;
        bool isDirectory;
        ImTextureID textureID = 0;
        bool attemptedLoad = false;
    };

    static std::vector<FileInfo> sCachedFiles;
    static bool sNeedsRefresh = true;
    static int sSelectedFileIndex = -1;

    static bool sRequestTreeSync = false;
    static std::unordered_set<const void*> sNodesToExpand;

    static std::vector<IFileSystemEntryPtr> sBreadcrumbPath;

    struct ThumbnailRequest {
        ArchivePtr archive;
        std::shared_ptr<FileEntry> entry;
        std::string extension;
        int fileIndex;
        uint64_t generation;
    };

    struct ThumbnailResult {
        int width;
        int height;
        std::vector<uint8_t> data;
        int fileIndex;
        uint64_t generation;
        bool success;
    };

    static std::deque<ThumbnailRequest> sLoadQueue;
    static std::deque<ThumbnailResult> sResultQueue;
    static std::mutex sQueueMutex;
    static std::atomic<bool> sWorkerRunning{ false };
    static std::thread sWorkerThread;
    static uint64_t sCurrentGeneration = 0;
    static std::condition_variable sQueueCV;

    static bool BuildPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target, std::vector<IFileSystemEntryPtr>& outPath)
    {
        if (current.get() == target.get())
        {
            outPath.push_back(current);
            return true;
        }

        if (!current->isDirectory()) return false;

        for (const auto& child : current->getChildren())
        {
            if (child && child->isDirectory())
            {
                if (BuildPathToNode(child, target, outPath))
                {
                    outPath.insert(outPath.begin(), current);
                    return true;
                }
            }
        }
        return false;
    }

    static void ThumbnailWorker()
    {
        while (sWorkerRunning)
        {
            ThumbnailRequest req;
            bool hasJob = false;

            {
                std::unique_lock<std::mutex> lock(sQueueMutex);
                sQueueCV.wait(lock, [] { return !sLoadQueue.empty() || !sWorkerRunning; });

                if (!sWorkerRunning) break;

                if (!sLoadQueue.empty())
                {
                    req = sLoadQueue.front();
                    sLoadQueue.pop_front();
                    hasJob = true;
                }
            }

            if (hasJob)
            {
                ThumbnailResult res;
                res.fileIndex = req.fileIndex;
                res.generation = req.generation;
                res.success = false;

                if (req.archive && req.entry)
                {
                    if (req.extension == ".area")
                    {
                        auto parsed = AreaFile::parseAreaFile(req.archive, req.entry);
                        if (parsed.valid)
                        {
                            int w = 256;
                            int h = 256;
                            std::vector<uint8_t> pixels(w * h * 4, 0);

                            float minH = parsed.minBounds.y;
                            float maxH = parsed.maxHeight;
                            float range = maxH - minH;
                            if (range < 1.0f) range = 1.0f;

                            for (int cz = 0; cz < 16; cz++)
                            {
                                for (int cx = 0; cx < 16; cx++)
                                {
                                    int chunkIdx = cz * 16 + cx;
                                    if (chunkIdx >= (int)parsed.chunks.size()) continue;

                                    const auto& chunk = parsed.chunks[chunkIdx];
                                    if (!chunk.valid || chunk.vertices.empty()) continue;

                                    for (int lz = 0; lz < 16; lz++)
                                    {
                                        for (int lx = 0; lx < 16; lx++)
                                        {
                                            int vIdx = lz * 17 + lx;
                                            if (vIdx >= (int)chunk.vertices.size()) continue;

                                            float height = chunk.vertices[vIdx].y;
                                            float norm = (height - minH) / range;
                                            uint8_t val = (uint8_t)(std::clamp(norm, 0.0f, 1.0f) * 255.0f);

                                            int px = cx * 16 + lx;
                                            int py = cz * 16 + lz;

                                            int pIdx = (py * w + px) * 4;

                                            pixels[pIdx + 0] = val;
                                            pixels[pIdx + 1] = val;
                                            pixels[pIdx + 2] = val;
                                            pixels[pIdx + 3] = 255;
                                        }
                                    }
                                }
                            }
                            res.width = w;
                            res.height = h;
                            res.data = std::move(pixels);
                            res.success = true;
                        }
                    }
                    else
                    {
                        std::vector<uint8_t> bytes;
                        if (req.archive->getFileData(req.entry, bytes) && !bytes.empty())
                        {
                            Tex::File tf;
                            if (tf.readFromMemory(bytes.data(), bytes.size()))
                            {
                                Tex::ImageRGBA img;
                                if (tf.decodeLargestMipToRGBA(img))
                                {
                                    res.width = img.width;
                                    res.height = img.height;
                                    res.data = std::move(img.rgba);
                                    res.success = true;
                                }
                            }
                        }
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(sQueueMutex);
                    sResultQueue.push_back(std::move(res));
                }
            }
        }
    }

    static void EnsureWorkerStarted()
    {
        if (!sWorkerRunning)
        {
            sWorkerRunning = true;
            sWorkerThread = std::thread(ThumbnailWorker);
            sWorkerThread.detach();
        }
    }

    static void ProcessThumbnailResults()
    {
        std::lock_guard<std::mutex> lock(sQueueMutex);
        while (!sResultQueue.empty())
        {
            auto res = sResultQueue.front();
            sResultQueue.pop_front();

            if (res.generation != sCurrentGeneration) continue;
            if (res.fileIndex < 0 || res.fileIndex >= (int)sCachedFiles.size()) continue;

            auto& file = sCachedFiles[res.fileIndex];
            if (res.success && !file.textureID)
            {
                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res.width, res.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, res.data.data());

                file.textureID = (ImTextureID)(intptr_t)tex;
            }
        }
    }

    static std::string GetExtension(const std::string& filename)
    {
        size_t dot = filename.rfind('.');
        if (dot != std::string::npos)
            return ToLowerCopy(filename.substr(dot));
        return "";
    }

    static bool FindPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target)
    {
        if (current.get() == target.get())
        {
            sNodesToExpand.insert(current.get());
            return true;
        }
        if (!current->isDirectory()) return false;

        for (const auto& child : current->getChildren())
        {
            if (child && child->isDirectory())
            {
                if (FindPathToNode(child, target))
                {
                    sNodesToExpand.insert(current.get());
                    return true;
                }
            }
        }
        return false;
    }

    static void CollectRecursive(const ArchivePtr& archive, const IFileSystemEntryPtr& folder, const std::string& filterLower, std::vector<FileInfo>& outList)
    {
        if (!folder || !folder->isDirectory()) return;

        for (const auto& child : folder->getChildren())
        {
            if (!child) continue;

            if (child->isDirectory())
            {
                CollectRecursive(archive, child, filterLower, outList);
            }
            else
            {
                std::string name = wstring_to_utf8(child->getEntryName());

                auto searchIt = std::search(
                    name.begin(), name.end(),
                    filterLower.begin(), filterLower.end(),
                    [](char c1, char c2) {
                        return std::tolower(static_cast<unsigned char>(c1)) == c2;
                    }
                );

                if (searchIt != name.end())
                {
                    FileInfo info;
                    info.name = std::move(name);
                    info.extension = GetExtension(info.name);
                    info.entry = child;
                    info.archive = archive;
                    info.isDirectory = false;
                    outList.push_back(std::move(info));
                }
            }
        }
    }

    static void RefreshFileList(AppState& state)
    {
        {
            std::lock_guard<std::mutex> lock(sQueueMutex);
            sCurrentGeneration++;
            sLoadQueue.clear();
            sResultQueue.clear();
        }

        for (const auto& file : sCachedFiles)
        {
            if (file.textureID)
            {
                GLuint tex = (GLuint)(intptr_t)file.textureID;
                glDeleteTextures(1, &tex);
            }
        }
        sCachedFiles.clear();

        sBreadcrumbPath.clear();
        if (sSelectedFolder && sSelectedArchive)
        {
            auto root = sSelectedArchive->getRoot();
            if (root)
            {
                BuildPathToNode(root, sSelectedFolder, sBreadcrumbPath);
            }
        }

        bool isFiltering = (sSearchFilter[0] != '\0');
        std::string filterLower;
        if (isFiltering) {
            filterLower = sSearchFilter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        }

        if (!sSelectedFolder)
        {
            for (const auto& archive : state.archives)
            {
                if (!archive) continue;
                auto root = archive->getRoot();
                if (!root) continue;

                if (isFiltering)
                {
                    CollectRecursive(archive, root, filterLower, sCachedFiles);
                }
                else
                {
                    FileInfo info;
                    std::filesystem::path p(archive->getPath());
                    info.name = p.filename().string();
                    info.extension = "";
                    info.entry = root;
                    info.archive = archive;
                    info.isDirectory = true;
                    sCachedFiles.push_back(info);
                }
            }
        }
        else if (sSelectedArchive)
        {
            if (isFiltering)
            {
                CollectRecursive(sSelectedArchive, sSelectedFolder, filterLower, sCachedFiles);
            }
            else
            {
                for (const auto& child : sSelectedFolder->getChildren())
                {
                    if (!child) continue;

                    FileInfo info;
                    info.name = wstring_to_utf8(child->getEntryName());
                    info.extension = GetExtension(info.name);
                    info.entry = child;
                    info.archive = sSelectedArchive;
                    info.isDirectory = child->isDirectory();

                    sCachedFiles.push_back(info);
                }
            }
        }

        std::sort(sCachedFiles.begin(), sCachedFiles.end(), [](const FileInfo& a, const FileInfo& b) {
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });

        sNeedsRefresh = false;
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
        gLoadedModel = nullptr;

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

    static void LoadSingleM3(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry)
    {
        if (!arc || !fileEntry) return;

        state.currentArea.reset();
        state.m3Render = nullptr;
        state.show_models_window = false;

        gPendingModelArchive = arc;

        std::string name = wstring_to_utf8(fileEntry->getEntryName());
        StartLoadingModel(arc, fileEntry, name);
    }

    static void HandleFileOpen(AppState& state, const FileInfo& file)
    {
        if (file.isDirectory)
        {
            sSelectedFolder = file.entry;
            sSelectedArchive = file.archive;
            sSearchFilter[0] = '\0';

            sRequestTreeSync = true;
            sNeedsRefresh = true;
            return;
        }

        auto fileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);
        if (!fileEntry) return;

        if (file.extension == ".area")
        {
            LoadSingleArea(state, file.archive, fileEntry);
        }
        else if (file.extension == ".m3")
        {
            LoadSingleM3(state, file.archive, fileEntry);
        }
        else if (file.extension == ".tex")
        {
            Tex::OpenTexPreviewFromEntry(state, file.archive, fileEntry);
        }
        else if (file.extension == ".tbl")
        {
            UI_Tables::OpenTblFile(state, file.archive, fileEntry);
        }
    }

    static void RenderTreeNode(AppState& state, const IFileSystemEntryPtr& entry, const ArchivePtr& archive, int depth = 0)
    {
        if (!entry || !entry->isDirectory()) return;

        std::string name = wstring_to_utf8(entry->getEntryName());
        if (name.empty()) name = "/";

        ImGui::PushID(static_cast<const void*>(entry.get()));

        bool isSelected = (sSelectedFolder.get() == entry.get());

        if (sRequestTreeSync && sNodesToExpand.count(entry.get()))
            ImGui::SetNextItemOpen(true);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        bool hasChildren = false;
        for (const auto& child : entry->getChildren())
        {
            if (child && child->isDirectory())
            {
                hasChildren = true;
                break;
            }
        }

        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool open = ImGui::TreeNodeEx(name.c_str(), flags);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            sSelectedFolder = entry;
            sSelectedArchive = archive;
            sSelectedPath = name;
            sSearchFilter[0] = '\0';
            sNeedsRefresh = true;
        }

        if (open)
        {
            std::vector<IFileSystemEntryPtr> dirs;
            for (const auto& child : entry->getChildren())
            {
                if (child && child->isDirectory())
                    dirs.push_back(child);
            }

            std::sort(dirs.begin(), dirs.end(), [](const IFileSystemEntryPtr& a, const IFileSystemEntryPtr& b) {
                return a->getEntryName() < b->getEntryName();
            });

            for (const auto& dir : dirs)
            {
                RenderTreeNode(state, dir, archive, depth + 1);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    static void RenderFileTree(AppState& state)
    {
        if (sRequestTreeSync)
        {
            sNodesToExpand.clear();
            if (sSelectedFolder && sSelectedArchive)
            {
                auto root = sSelectedArchive->getRoot();
                if (root)
                    FindPathToNode(root, sSelectedFolder);
            }
        }

        if (ImGui::BeginChild("FileTree", ImVec2(sTreeWidth, 0), true))
        {
            bool stylePushed = false;
            if (sIsDocked) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
                stylePushed = true;
            }
            if (ImGui::Button("Dock", ImVec2(-FLT_MIN, 0.0f)))
            {
                sIsDocked = !sIsDocked;
                if (sIsDocked) sIsOpen = true;
            }
            if (stylePushed) {
                ImGui::PopStyleColor();
            }

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 2.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 15.0f);

            ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
            if (sSelectedFolder == nullptr)
                rootFlags |= ImGuiTreeNodeFlags_Selected;

            bool rootOpen = ImGui::TreeNodeEx("Contents", rootFlags);

            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                sSelectedFolder = nullptr;
                sSelectedArchive = nullptr;
                sSelectedPath.clear();
                sSearchFilter[0] = '\0';
                sNeedsRefresh = true;
            }

            if (rootOpen)
            {
                for (auto& archive : state.archives)
                {
                    if (!archive) continue;
                    auto root = archive->getRoot();
                    if (!root) continue;

                    std::filesystem::path p(archive->getPath());
                    std::string archiveName = p.filename().string();

                    ImGui::PushID(archive.get());

                    if (sRequestTreeSync && sSelectedArchive == archive)
                        ImGui::SetNextItemOpen(true);

                    ImGuiTreeNodeFlags arcFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
                    if (sSelectedFolder.get() == root.get())
                        arcFlags |= ImGuiTreeNodeFlags_Selected;

                    bool nodeOpen = ImGui::TreeNodeEx(archiveName.c_str(), arcFlags);

                    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    {
                        sSelectedFolder = root;
                        sSelectedArchive = archive;
                        sSelectedPath = archiveName;
                        sSearchFilter[0] = '\0';
                        sNeedsRefresh = true;
                    }

                    if (nodeOpen)
                    {
                        std::vector<IFileSystemEntryPtr> dirs;
                        for (const auto& child : root->getChildren())
                        {
                            if (child && child->isDirectory())
                                dirs.push_back(child);
                        }

                        std::sort(dirs.begin(), dirs.end(), [](const IFileSystemEntryPtr& a, const IFileSystemEntryPtr& b) {
                            return a->getEntryName() < b->getEntryName();
                        });

                        for (const auto& dir : dirs)
                        {
                            RenderTreeNode(state, dir, archive);
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            ImGui::PopStyleVar(3);
        }
        ImGui::EndChild();

        sRequestTreeSync = false;
    }

    static void RenderFileBrowser(AppState& state)
    {
        if (sNeedsRefresh)
        {
            RefreshFileList(state);
            sSelectedFileIndex = -1;
        }

        ProcessThumbnailResults();

        if (!sFolderIcon)
            sFolderIcon = UI_Utils::LoadTexture("Assets/Icons/Folder.png");
        if (!sTblIcon)
            sTblIcon = UI_Utils::LoadTexture("Assets/Icons/Tbl.png");
        if (!sAreaIcon)
            sAreaIcon = UI_Utils::LoadTexture("Assets/Icons/Area.png");
        if (!sAudioIcon)
            sAudioIcon = UI_Utils::LoadTexture("Assets/Icons/Audio.png");

        if (ImGui::BeginChild("FileBrowser", ImVec2(0, 0), true))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1,1,1,0.2f));

            if (ImGui::Button("Contents"))
            {
                sSelectedFolder = nullptr;
                sSelectedArchive = nullptr;
                sSelectedPath.clear();
                sSearchFilter[0] = '\0';
                sNeedsRefresh = true;
                sRequestTreeSync = true;
            }

            if (sSelectedArchive)
            {
                ImGui::SameLine();
                ImGui::Text("/");
                ImGui::SameLine();

                std::string archiveName = std::filesystem::path(sSelectedArchive->getPath()).filename().string();

                if (ImGui::Button(archiveName.c_str()))
                {
                    sSelectedFolder = sSelectedArchive->getRoot();
                    sSearchFilter[0] = '\0';
                    sNeedsRefresh = true;
                    sRequestTreeSync = true;
                }

                for (size_t i = 1; i < sBreadcrumbPath.size(); ++i)
                {
                    ImGui::SameLine();
                    ImGui::Text("/");
                    ImGui::SameLine();

                    auto& entry = sBreadcrumbPath[i];
                    std::string name = wstring_to_utf8(entry->getEntryName());
                    if (name.empty()) name = "???";

                    if (i == sBreadcrumbPath.size() - 1)
                    {
                        ImGui::TextDisabled("%s", name.c_str());
                    }
                    else
                    {
                        if (ImGui::Button(name.c_str()))
                        {
                            sSelectedFolder = entry;
                            sSearchFilter[0] = '\0';
                            sNeedsRefresh = true;
                            sRequestTreeSync = true;
                        }
                    }
                }
            }
            ImGui::PopStyleColor(3);

            ImGui::SetNextItemWidth(240.0f);
            if (ImGui::InputTextWithHint("##filter", "Search...", sSearchFilter, IM_ARRAYSIZE(sSearchFilter)))
            {
                sNeedsRefresh = true;
            }

            ImGui::Separator();

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            float windowVisibleX = ImGui::GetContentRegionAvail().x;
            float spacingX = 8.0f;
            float spacingY = 8.0f;

            float iconSize = 90.0f;
            float highlightPad = 6.0f;

            float textLineHeight = ImGui::GetTextLineHeightWithSpacing();
            int nameLines = 3;
            int extLines = 1;
            float textBlockHeight = textLineHeight * (nameLines + extLines);

            float contentWidth = iconSize;
            float contentHeight = iconSize + textBlockHeight;

            float cellWidth = contentWidth + (highlightPad * 2);
            float cellHeight = contentHeight + (highlightPad * 2);

            int columns = std::max(1, static_cast<int>((windowVisibleX + spacingX) / (cellWidth + spacingX)));

            int numFiles = (int)sCachedFiles.size();
            int rows = (numFiles + columns - 1) / columns;

            ImVec2 startPos = ImGui::GetCursorScreenPos();
            float startX = startPos.x;

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
            {
                sSelectedFileIndex = -1;
            }

            ImGuiListClipper clipper;
            clipper.Begin(rows, cellHeight + spacingY);

            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    float currentY = startPos.y + (row * (cellHeight + spacingY));

                    for (int col = 0; col < columns; col++)
                    {
                        int i = row * columns + col;
                        if (i >= numFiles) break;

                        auto& file = sCachedFiles[i];

                        float currentX = startX + (col * (cellWidth + spacingX));

                        ImVec2 cellMin(currentX, currentY);
                        ImVec2 cellMax(currentX + cellWidth, currentY + cellHeight);
                        ImVec2 contentMin = cellMin + ImVec2(highlightPad, highlightPad);

                        ImGui::SetCursorScreenPos(cellMin);
                        ImGui::PushID(static_cast<int>(i));

                        if (ImGui::InvisibleButton("##hit", ImVec2(cellWidth, cellHeight)))
                        {
                            sSelectedFileIndex = static_cast<int>(i);
                        }

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            HandleFileOpen(state, file);
                        }

                        bool isSelected = (sSelectedFileIndex == static_cast<int>(i));
                        bool isHovered = ImGui::IsItemHovered();

                        if (isSelected)
                        {
                            drawList->AddRect(cellMin, cellMax, IM_COL32(100, 200, 255, 255), 4.0f, 0, 2.0f);
                            drawList->AddRectFilled(cellMin, cellMax, IM_COL32(100, 200, 255, 40), 4.0f);
                        }
                        else if (isHovered)
                        {
                            drawList->AddRectFilled(cellMin, cellMax, IM_COL32(255, 255, 255, 20), 4.0f);
                        }

                        bool isTex = (file.extension == ".tex");
                        bool isArea = (file.extension == ".area");

                        if ((isTex || isArea) && !file.attemptedLoad && !file.textureID)
                        {
                            file.attemptedLoad = true;
                            auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                            if (fe)
                            {
                                ThumbnailRequest req;
                                req.archive = file.archive;
                                req.entry = fe;
                                req.fileIndex = i;
                                req.extension = file.extension;
                                req.generation = sCurrentGeneration;

                                {
                                    std::lock_guard<std::mutex> lock(sQueueMutex);
                                    sLoadQueue.push_back(req);
                                }
                                sQueueCV.notify_one();
                            }
                        }

                        if (file.isDirectory && sFolderIcon)
                        {
                            drawList->AddImage(sFolderIcon, contentMin, contentMin + ImVec2(iconSize, iconSize));
                        }
                        else if (file.textureID)
                        {
                            drawList->AddImage(file.textureID, contentMin, contentMin + ImVec2(iconSize, iconSize));
                        }
                        else if (file.extension == ".tbl" && sTblIcon)
                        {
                            drawList->AddImage(sTblIcon, contentMin, contentMin + ImVec2(iconSize, iconSize));
                        }
                        else if (file.extension == ".area" && sAreaIcon)
                        {
                            drawList->AddImage(sAreaIcon, contentMin, contentMin + ImVec2(iconSize, iconSize));
                        }
                        else if (file.extension == ".wem" && sAudioIcon)
                        {
                            drawList->AddImage(sAudioIcon, contentMin, contentMin + ImVec2(iconSize, iconSize));
                        }
                        else
                        {
                            ImU32 bgColor = IM_COL32(51, 51, 51, 255);
                            if (file.isDirectory)              bgColor = IM_COL32(76, 64, 38, 255);
                            else if (file.extension == ".m3")  bgColor = IM_COL32(38, 76, 38, 255);
                            else if (file.extension == ".area") bgColor = IM_COL32(38, 51, 76, 255);
                            else if (file.extension == ".tex")  bgColor = IM_COL32(76, 38, 76, 255);
                            else if (file.extension == ".tbl")  bgColor = IM_COL32(76, 76, 38, 255);
                            else if (file.extension == ".wem")  bgColor = IM_COL32(76, 76, 76, 255);

                            drawList->AddRectFilled(contentMin + ImVec2(10, 10), contentMin + ImVec2(iconSize - 10, iconSize - 10), bgColor, 4.0f);
                        }

                        ImVec2 textStart = contentMin + ImVec2(0, iconSize);
                        ImGui::SetCursorScreenPos(textStart + ImVec2(2, 0));

                        std::string stem = file.name;
                        if (!file.isDirectory)
                        {
                            size_t lastDot = stem.rfind('.');
                            if (lastDot != std::string::npos)
                                stem = stem.substr(0, lastDot);
                        }

                        ImVec4 clipRect(cellMin.x, cellMin.y, cellMax.x, textStart.y + (textLineHeight * nameLines));
                        drawList->PushClipRect(ImVec2(clipRect.x, clipRect.y), ImVec2(clipRect.z, clipRect.w), true);

                        ImVec4 nameColor(1.0f, 1.0f, 1.0f, 1.0f);
                        if (file.extension == ".m3")
                            nameColor = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
                        else if (file.extension == ".tex")
                            nameColor = ImVec4(0.8f, 0.5f, 0.9f, 1.0f);
                        else if (file.extension == ".area")
                            nameColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
                        else if (file.extension == ".wem")
                            nameColor = ImVec4(0.9f, 0.35f, 0.35f, 1.0f);

                        ImGui::PushStyleColor(ImGuiCol_Text, nameColor);
                        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize - 4);
                        ImGui::TextUnformatted(stem.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::PopStyleColor();

                        drawList->PopClipRect();

                        if (!file.isDirectory)
                        {
                            ImVec2 extPos = textStart + ImVec2(2, textLineHeight * nameLines);
                            ImGui::SetCursorScreenPos(extPos);

                            std::string typeStr;
                            if (file.extension.size() > 1)
                            {
                                typeStr = file.extension.substr(1);
                                if (!typeStr.empty()) typeStr[0] = toupper(typeStr[0]);
                            }
                            else { typeStr = "File"; }

                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                            ImGui::TextUnformatted(typeStr.c_str());
                            ImGui::PopStyleColor();
                        }

                        ImGui::PopID();
                    }
                }
            }
            clipper.End();

            float totalH = startPos.y + rows * (cellHeight + spacingY);
            ImGui::SetCursorScreenPos(ImVec2(startX, totalH));
            ImGui::Dummy(ImVec2(1, 20.0f));

            if (numFiles == 0)
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No files found");
            }
        }
        ImGui::EndChild();
    }

    void Toggle() { sIsOpen = !sIsOpen; }
    bool IsOpen() { return sIsOpen; }
    bool IsDocked() { return sIsDocked; }
    void HideIfNotDocked() { if (!sIsDocked) sIsOpen = false; }

    static const float BOTTOM_BAR_HEIGHT = 34.0f;
    float GetHeight() { return sCurrentHeight + BOTTOM_BAR_HEIGHT; }
    float GetBarHeight() { return BOTTOM_BAR_HEIGHT; }

    void Reset()
    {
        sSelectedFolder = nullptr;
        sSelectedArchive = nullptr;
        sSelectedPath.clear();
        sSearchFilter[0] = '\0';

        {
            std::lock_guard<std::mutex> lock(sQueueMutex);
            sCurrentGeneration++;
            sLoadQueue.clear();
            sResultQueue.clear();
        }

        for (const auto& file : sCachedFiles)
        {
            if (file.textureID)
            {
                GLuint tex = (GLuint)(intptr_t)file.textureID;
                glDeleteTextures(1, &tex);
            }
        }

        sCachedFiles.clear();
        sNeedsRefresh = true;
    }

    void Draw(AppState& state)
    {
        EnsureWorkerStarted();

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Space, false))
        {
            Toggle();
        }

        if (!sContentBrowserIcon)
            sContentBrowserIcon = UI_Utils::LoadTexture("Assets/Icons/ContentBrowser.png");

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float dt = ImGui::GetIO().DeltaTime;

        float targetH = sIsOpen ? sTargetHeight : 0.0f;
        if (sIsDocked) targetH = sTargetHeight;

        if (sCurrentHeight < targetH)
        {
            sCurrentHeight += sAnimSpeed * dt;
            if (sCurrentHeight > targetH)
                sCurrentHeight = targetH;
        }
        else if (sCurrentHeight > targetH)
        {
            sCurrentHeight -= sAnimSpeed * dt;
            if (sCurrentHeight < targetH)
                sCurrentHeight = targetH;
        }

        float totalHeight = sCurrentHeight + BOTTOM_BAR_HEIGHT;
        float windowY = viewport->Pos.y + viewport->Size.y - totalHeight;

        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, windowY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, totalHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.98f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin("##ContentBrowser", nullptr, flags))
        {
            float handleHeight = 6.0f;

            if (sCurrentHeight > 1.0f)
            {
                ImVec2 handlePos = ImGui::GetCursorScreenPos();
                ImVec2 handleSize = ImVec2(ImGui::GetContentRegionAvail().x, handleHeight);

                ImGui::InvisibleButton("##ResizeHandle", handleSize);

                bool isHovered = ImGui::IsItemHovered();
                bool isActive = ImGui::IsItemActive();

                ImU32 handleColor = isActive ? IM_COL32(100, 149, 237, 255) :
                                  (isHovered ? IM_COL32(80, 80, 100, 255) : IM_COL32(60, 60, 70, 255));

                ImGui::GetWindowDrawList()->AddRectFilled(
                    handlePos,
                    ImVec2(handlePos.x + handleSize.x, handlePos.y + handleSize.y),
                    handleColor,
                    3.0f
                );

                if (isActive)
                {
                    float mouseDelta = -ImGui::GetIO().MouseDelta.y;
                    sTargetHeight += mouseDelta;
                    sTargetHeight = std::clamp(sTargetHeight, 150.0f, viewport->Size.y * 0.7f);
                    sCurrentHeight = sTargetHeight;
                }

                if (isHovered || isActive)
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

                float contentH = ImGui::GetContentRegionAvail().y - BOTTOM_BAR_HEIGHT;
                if (contentH > 0)
                {
                    if (ImGui::BeginChild("##ContentArea", ImVec2(0, contentH), false, ImGuiWindowFlags_NoScrollbar))
                    {
                        float availW = ImGui::GetContentRegionAvail().x;
                        float splitterW = 6.0f;
                        float minTree = 140.0f;
                        float minRight = 200.0f;
                        float maxTree = std::max(minTree, availW - splitterW - minRight);
                        sTreeWidth = std::clamp(sTreeWidth, minTree, maxTree);

                        RenderFileTree(state);

                        ImGui::SameLine(0.0f, 0.0f);

                        ImVec2 splitPos = ImGui::GetCursorScreenPos();
                        ImGui::InvisibleButton("##TreeSplitter", ImVec2(splitterW, -1.0f));

                        bool splitHovered = ImGui::IsItemHovered();
                        bool splitActive = ImGui::IsItemActive();

                        ImU32 splitCol = splitActive ? IM_COL32(120, 170, 255, 255) :
                                       (splitHovered ? IM_COL32(90, 90, 110, 255) : IM_COL32(70, 70, 85, 255));

                        ImGui::GetWindowDrawList()->AddRectFilled(
                            splitPos,
                            ImVec2(splitPos.x + splitterW, splitPos.y + ImGui::GetItemRectSize().y),
                            splitCol
                        );

                        if (splitHovered || splitActive)
                            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                        if (splitActive)
                        {
                            sTreeWidth += ImGui::GetIO().MouseDelta.x;
                            sTreeWidth = std::clamp(sTreeWidth, minTree, maxTree);
                        }

                        ImGui::SameLine(0.0f, 0.0f);

                        RenderFileBrowser(state);
                    }
                    ImGui::EndChild();
                }

                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::Dummy(ImVec2(0, handleHeight));
            }

            ImGui::SetCursorPosY(totalHeight - BOTTOM_BAR_HEIGHT);
            ImGui::Separator();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

            const char* btnText = "Content Browser";
            float textWidth = ImGui::CalcTextSize(btnText).x;
            float buttonWidth = 140.0f;
            float iconSize = ImGui::GetTextLineHeight();
            float spacing = 4.0f;
            float totalContentW = textWidth + (sContentBrowserIcon ? (iconSize + spacing) : 0.0f);

            float scale = 1.0f;
            if (totalContentW > buttonWidth - 10.0f) {
                scale = (buttonWidth - 10.0f) / totalContentW;
                ImGui::SetWindowFontScale(scale);
                iconSize *= scale;
                spacing *= scale;
            }

            float currentBtnHeight = ImGui::GetFrameHeight();
            float barTopY = totalHeight - BOTTOM_BAR_HEIGHT;
            float centeredY = barTopY + (BOTTOM_BAR_HEIGHT - currentBtnHeight) * 0.5f;

            ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().WindowPadding.x + 50.0f, centeredY));
            ImVec2 startPos = ImGui::GetCursorScreenPos();

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (buttonWidth - totalContentW) * 0.5f);

            float imgOffset = (currentBtnHeight - iconSize) * 0.5f;

            if (sContentBrowserIcon) {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + imgOffset);
                ImGui::Image((void*)(intptr_t)sContentBrowserIcon, ImVec2(iconSize, iconSize), ImVec2(0,0), ImVec2(1,1));
                ImGui::SameLine(0, spacing);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - imgOffset);
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(btnText);

            if (scale != 1.0f) {
                ImGui::SetWindowFontScale(1.0f);
            }

            ImGui::SetCursorScreenPos(startPos);
            if (ImGui::Button("##ContentBrowserBtn", ImVec2(buttonWidth, currentBtnHeight)))
            {
                Toggle();
            }

            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }
}