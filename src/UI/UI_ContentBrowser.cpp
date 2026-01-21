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

extern void SnapCameraToLoaded(AppState& state);
extern void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax);

namespace UI_ContentBrowser {
    static bool sIsOpen = false;
    static float sCurrentHeight = 0.0f;
    static float sTargetHeight = 300.0f;
    static float sAnimSpeed = 1500.0f;
    static float sTreeWidth = 200.0f;
    static ImTextureID sFolderIcon = 0;

    static IFileSystemEntryPtr sSelectedFolder = nullptr;
    static ArchivePtr sSelectedArchive = nullptr;
    static std::string sSelectedPath;

    struct FileInfo
    {
        std::string name;
        std::string extension;
        IFileSystemEntryPtr entry;
        ArchivePtr archive;
        bool isDirectory;
    };

    static std::vector<FileInfo> sCachedFiles;
    static bool sNeedsRefresh = true;
    static int sSelectedFileIndex = -1;

    static bool sRequestTreeSync = false;
    static std::unordered_set<const void*> sNodesToExpand;

    static std::string GetExtension(const std::string& filename)
    {
        size_t dot = filename.rfind('.');
        if (dot != std::string::npos)
            return ToLowerCopy(filename.substr(dot));
        return "";
    }

    static bool FindPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target)
    {
        if (current.get() == target.get()) return true;
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

    static void RefreshFileList(AppState& state)
    {
        sCachedFiles.clear();

        if (!sSelectedFolder)
        {
            for (const auto& archive : state.archives)
            {
                if (!archive) continue;
                auto root = archive->getRoot();
                if (!root) continue;

                FileInfo info;
                std::filesystem::path p(archive->getPath());
                info.name = p.filename().string();
                info.extension = "";
                info.entry = root;
                info.archive = archive;
                info.isDirectory = true;

                sCachedFiles.push_back(info);
            }

            std::sort(sCachedFiles.begin(), sCachedFiles.end(), [](const FileInfo& a, const FileInfo& b) {
                return a.name < b.name;
            });

            sNeedsRefresh = false;
            return;
        }

        if (!sSelectedArchive)
        {
            sNeedsRefresh = false;
            return;
        }

        const auto& children = sSelectedFolder->getChildren();

        for (const auto& child : children)
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
            sRequestTreeSync = true;

            std::string entryName = wstring_to_utf8(file.entry->getEntryName());
            if (entryName.empty() && sSelectedPath.empty())
                sSelectedPath = file.name;
            else
                sSelectedPath = entryName;

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
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 2.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 15.0f);

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

                if (ImGui::TreeNodeEx(archiveName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
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

        if (!sFolderIcon)
            sFolderIcon = UI_Utils::LoadTexture("Assets/Icons/Folder.png");

        if (ImGui::BeginChild("FileBrowser", ImVec2(0, 0), true))
        {
            if (!sSelectedPath.empty())
            {
                if (ImGui::Button("Root"))
                {
                    sSelectedFolder = nullptr;
                    sSelectedArchive = nullptr;
                    sSelectedPath.clear();
                    sNeedsRefresh = true;
                    sRequestTreeSync = true;
                }
                ImGui::SameLine();
                ImGui::Text("/ %s", sSelectedPath.c_str());
                ImGui::Separator();
            }
            else
            {
                ImGui::TextDisabled("All Archives");
                ImGui::Separator();
            }

            ImGuiStyle& style = ImGui::GetStyle();
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

            ImVec2 startPos = ImGui::GetCursorScreenPos();
            float startX = startPos.x;
            float currentY = startPos.y;

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
            {
                sSelectedFileIndex = -1;
            }

            for (size_t i = 0; i < sCachedFiles.size(); ++i)
            {
                const auto& file = sCachedFiles[i];

                int col = i % columns;
                int row = static_cast<int>(i) / columns;

                if (col == 0 && row > 0)
                {
                    currentY += cellHeight + spacingY;
                }

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

                if (file.isDirectory && sFolderIcon)
                {
                    drawList->AddImage(sFolderIcon, contentMin, contentMin + ImVec2(iconSize, iconSize));
                }
                else
                {
                    ImU32 bgColor = IM_COL32(51, 51, 51, 255);
                    if (file.isDirectory)           bgColor = IM_COL32(76, 64, 38, 255);
                    else if (file.extension == ".m3") bgColor = IM_COL32(38, 76, 38, 255);
                    else if (file.extension == ".area") bgColor = IM_COL32(38, 51, 76, 255);
                    else if (file.extension == ".tex")  bgColor = IM_COL32(76, 38, 76, 255);
                    else if (file.extension == ".tbl")  bgColor = IM_COL32(76, 76, 38, 255);

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

                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize - 4);
                ImGui::TextUnformatted(stem.c_str());
                ImGui::PopTextWrapPos();

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

            if (!sCachedFiles.empty())
            {
                ImGui::SetCursorScreenPos(ImVec2(startX, currentY + cellHeight + spacingY));
                ImGui::Dummy(ImVec2(1, 1)); 
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No files found");
            }
        }
        ImGui::EndChild();
    }

    void Toggle()
    {
        sIsOpen = !sIsOpen;
    }

    bool IsOpen()
    {
        return sIsOpen;
    }

    static const float BOTTOM_BAR_HEIGHT = 28.0f;

    float GetHeight()
    {
        return sCurrentHeight + BOTTOM_BAR_HEIGHT;
    }

    float GetBarHeight()
    {
        return BOTTOM_BAR_HEIGHT;
    }

    void Reset()
    {
        sSelectedFolder = nullptr;
        sSelectedArchive = nullptr;
        sSelectedPath.clear();
        sCachedFiles.clear();
        sNeedsRefresh = true;
    }

    void Draw(AppState& state)
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Space, false))
        {
            Toggle();
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float dt = ImGui::GetIO().DeltaTime;

        float targetH = sIsOpen ? sTargetHeight : 0.0f;

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

        if (sCurrentHeight < 1.0f)
            return;

        float windowY = viewport->Pos.y + viewport->Size.y - sCurrentHeight;

        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, windowY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, sCurrentHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.98f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin("##ContentBrowser", nullptr, flags))
        {
            float handleHeight = 6.0f;
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

            RenderFileTree(state);
            ImGui::SameLine();
            RenderFileBrowser(state);

            ImGui::PopStyleVar();
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }
}