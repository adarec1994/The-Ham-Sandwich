#define IMGUI_DEFINE_MATH_OPERATORS
#include "UI_ContentBrowser.h"
#include "FileOps.h"
#include "UI_Globals.h"
#include "UI_Utils.h"
#include "UI_Tables.h"
#include "../resource.h"
#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../Area/Heightmap.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include "../export/M3Export.h"
#include "../export/TerrainExport.h"
#include "../tex/tex.h"
#include "../Audio/AudioPlayer.h"
#include "../Audio/AudioPlayerWidget.h"
#include "../export/AudioExport.h"
#include "../Database/Tbl.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuiFileDialog.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <unordered_set>
#include <d3d11.h>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <cstring>

extern void SnapCameraToLoaded(AppState& state);
extern void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax);
extern ID3D11Device* gDevice;
extern ID3D11DeviceContext* gContext;

namespace UI_ContentBrowser {

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
            sFolderIcon = UI_Utils::LoadTextureFromResource(IDR_ICON_FOLDER);
        if (!sTblIcon)
            sTblIcon = UI_Utils::LoadTextureFromResource(IDR_ICON_TBL);
        if (!sAreaIcon)
            sAreaIcon = UI_Utils::LoadTextureFromResource(IDR_ICON_AREA);
        if (!sAudioIcon)
            sAudioIcon = UI_Utils::LoadTextureFromResource(IDR_ICON_AUDIO);

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
                sBnkViewActive = false;
                sBnkViewData.clear();
                sBnkWemEntries.clear();
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
                    sBnkViewActive = false;
                    sBnkViewData.clear();
                    sBnkWemEntries.clear();
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

                    bool isLast = (i == sBreadcrumbPath.size() - 1) && !sBnkViewActive;

                    if (isLast)
                    {
                        ImGui::TextDisabled("%s", name.c_str());
                    }
                    else
                    {
                        if (ImGui::Button(name.c_str()))
                        {
                            sSelectedFolder = entry;
                            sSearchFilter[0] = '\0';
                            sBnkViewActive = false;
                            sBnkViewData.clear();
                            sBnkWemEntries.clear();
                            sNeedsRefresh = true;
                            sRequestTreeSync = true;
                        }
                    }
                }

                if (sBnkViewActive)
                {
                    ImGui::SameLine();
                    ImGui::Text("/");
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", sBnkViewName.c_str());
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

                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
                            ImGui::BeginTooltip();
                            ImGui::TextUnformatted(file.name.c_str());
                            ImGui::EndTooltip();
                            ImGui::PopStyleVar();
                        }

                        if (file.extension == ".m3" && !file.isDirectory)
                        {

                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##m3context"))
                            {
                                std::string baseName = file.name;
                                size_t dotPos = baseName.rfind('.');
                                if (dotPos != std::string::npos)
                                    baseName = baseName.substr(0, dotPos);

                                bool canExport = !sExportInProgress.load();

                                if (!canExport)
                                    ImGui::BeginDisabled();

                                if (ImGui::MenuItem("Export as GLB"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".glb";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportGLBDlg", "Export GLB", ".glb", config);
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Export as FBX"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".fbx";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportFBXDlg", "Export FBX", ".fbx", config);
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Extract Raw (.m3)"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".m3";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExtractM3RawDlg", "Extract Raw M3", ".m3", config);
                                }

                                if (!canExport)
                                {
                                    ImGui::EndDisabled();
                                    ImGui::Separator();
                                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Export in progress...");
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
                        }

                        if (file.extension == ".tex" && !file.isDirectory)
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##texcontext"))
                            {
                                std::string baseName = file.name;
                                size_t dotPos = baseName.rfind('.');
                                if (dotPos != std::string::npos)
                                    baseName = baseName.substr(0, dotPos);

                                if (ImGui::MenuItem("Extract Raw (.tex)"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".tex";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExtractTexRawDlg", "Extract Raw Texture", ".tex", config);
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Export as PNG"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".png";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportTexPNGDlg", "Export PNG", ".png", config);
                                }

                                if (ImGui::MenuItem("Export as JPG"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".jpg";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportTexJPGDlg", "Export JPG", ".jpg", config);
                                }

                                if (ImGui::MenuItem("Export as TIFF"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".tiff";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportTexTIFFDlg", "Export TIFF", ".tiff", config);
                                }

                                if (ImGui::MenuItem("Export as DDS"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".dds";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportTexDDSDlg", "Export DDS", ".dds", config);
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
                        }

                        if (file.extension == ".area" && !file.isDirectory)
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##areacontext"))
                            {
                                std::string baseName = file.name;
                                size_t dotPos = baseName.rfind('.');
                                if (dotPos != std::string::npos)
                                    baseName = baseName.substr(0, dotPos);

                                if (ImGui::MenuItem("View Heightmap"))
                                {
                                    auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                                    if (fe)
                                        Heightmap::ViewSingleAreaHeightmap(state, file.archive, fe, baseName);
                                }

                                if (ImGui::BeginMenu("Extract Heightmap"))
                                {
                                    auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                                    if (ImGui::MenuItem("PNG"))
                                    {
                                        if (fe) Heightmap::SetPendingSingleHeightmapExport(file.archive, fe, baseName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + "_heightmap.png";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapPngDlg", "Export Heightmap as PNG", ".png", config);
                                    }
                                    if (ImGui::MenuItem("JPEG"))
                                    {
                                        if (fe) Heightmap::SetPendingSingleHeightmapExport(file.archive, fe, baseName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + "_heightmap.jpg";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapJpgDlg", "Export Heightmap as JPEG", ".jpg", config);
                                    }
                                    if (ImGui::MenuItem("BMP"))
                                    {
                                        if (fe) Heightmap::SetPendingSingleHeightmapExport(file.archive, fe, baseName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + "_heightmap.bmp";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapBmpDlg", "Export Heightmap as BMP", ".bmp", config);
                                    }
                                    if (ImGui::MenuItem("TGA"))
                                    {
                                        if (fe) Heightmap::SetPendingSingleHeightmapExport(file.archive, fe, baseName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + "_heightmap.tga";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapTgaDlg", "Export Heightmap as TGA", ".tga", config);
                                    }
                                    ImGui::EndMenu();
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Extract Raw (.area)"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".area";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExtractAreaRawDlg", "Extract Raw Area", ".area", config);
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Export as .wsterrain"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExportWsTerrainDlg", "Export Terrain", nullptr, config);
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
                        }

                        if (file.extension == ".wem" && !file.isDirectory)
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##wemcontext"))
                            {
                                std::string baseName = file.name;
                                size_t dotPos = baseName.rfind('.');
                                if (dotPos != std::string::npos)
                                    baseName = baseName.substr(0, dotPos);

                                auto getWemData = [&](std::vector<uint8_t>& outData) -> bool {
                                    if (file.entry) {
                                        auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                                        if (fe && file.archive->getFileData(fe, outData) && !outData.empty())
                                            return true;
                                    } else if (sBnkViewActive && !sBnkViewData.empty()) {
                                        const BnkWemEntry* foundEntry = nullptr;
                                        for (const auto& entry : sBnkWemEntries) {
                                            if (entry.displayName == baseName) {
                                                foundEntry = &entry;
                                                break;
                                            }
                                        }

                                        if (!foundEntry) return false;

                                        const uint8_t* data = sBnkViewData.data();
                                        size_t size = sBnkViewData.size();
                                        size_t pos = 0;
                                        const uint8_t* dataSection = nullptr;

                                        while (pos + 8 <= size) {
                                            char sectionId[5] = {0};
                                            memcpy(sectionId, data + pos, 4);
                                            uint32_t sectionSize = *(uint32_t*)(data + pos + 4);

                                            if (memcmp(sectionId, "DATA", 4) == 0) {
                                                dataSection = data + pos + 8;
                                                break;
                                            }
                                            pos += 8 + sectionSize;
                                        }

                                        if (dataSection) {
                                            outData.assign(dataSection + foundEntry->offset,
                                                           dataSection + foundEntry->offset + foundEntry->size);
                                            return true;
                                        }
                                    }
                                    return false;
                                };

                                if (ImGui::MenuItem("Play"))
                                {
                                    std::vector<uint8_t> wemData;
                                    if (getWemData(wemData))
                                    {
                                        Audio::AudioPlayerWidget::Get().PlayFile(wemData.data(), wemData.size(), file.name);
                                    }
                                }

                                if (ImGui::MenuItem("Stop"))
                                {
                                    Audio::AudioManager::Get().StopAll();
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Extract Raw (.wem)"))
                                {
                                    std::vector<uint8_t> wemData;
                                    if (getWemData(wemData))
                                    {
                                        sAudioExportData = std::move(wemData);
                                        sAudioExportName = baseName;

                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + ".wem";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExtractBnkWemRawDlg", "Extract Raw WEM", ".wem", config);
                                    }
                                }

                                if (ImGui::MenuItem("Convert to WAV"))
                                {
                                    if (getWemData(sAudioExportData))
                                    {
                                        sAudioExportName = baseName;
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + ".wav";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportAudioDlg", "Save WAV", ".wav", config);
                                    }
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
                        }

                        if (file.extension == ".bnk" && !file.isDirectory)
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##bnkcontext"))
                            {
                                std::string baseName = file.name;
                                size_t dotPos = baseName.rfind('.');
                                if (dotPos != std::string::npos)
                                    baseName = baseName.substr(0, dotPos);

                                if (ImGui::MenuItem("Extract WEMs..."))
                                {
                                    auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                                    if (fe && file.archive->getFileData(fe, sBnkExportData) && !sBnkExportData.empty())
                                    {
                                        LoadWemNameLookup(state.archives);

                                        sBnkExportName = baseName;
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExtractBnkDlg", "Select Output Folder", nullptr, config);
                                    }
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Extract Raw (.bnk)"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".bnk";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExtractBnkRawDlg", "Extract Raw BNK", ".bnk", config);
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
                        }

                        if (file.extension == ".tbl" && !file.isDirectory)
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##tblcontext"))
                            {
                                std::string baseName = file.name;
                                size_t dotPos = baseName.rfind('.');
                                if (dotPos != std::string::npos)
                                    baseName = baseName.substr(0, dotPos);

                                if (ImGui::MenuItem("Export as CSV"))
                                {
                                    auto fe = std::dynamic_pointer_cast<FileEntry>(file.entry);
                                    if (fe && file.archive->getFileData(fe, sTblExportData) && !sTblExportData.empty())
                                    {
                                        sTblExportName = baseName;
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = baseName + ".csv";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportTblCsvDlg", "Export CSV", ".csv", config);
                                    }
                                }

                                ImGui::Separator();

                                if (ImGui::MenuItem("Extract Raw (.tbl)"))
                                {
                                    sExportDefaultName = baseName;
                                    sExportArchive = file.archive;
                                    sExportFileEntry = std::dynamic_pointer_cast<FileEntry>(file.entry);

                                    IGFD::FileDialogConfig config;
                                    config.path = ".";
                                    config.fileName = baseName + ".tbl";
                                    config.flags = ImGuiFileDialogFlags_Modal;
                                    ImGuiFileDialog::Instance()->OpenDialog("ExtractTblRawDlg", "Extract Raw TBL", ".tbl", config);
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
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

                        if (file.isLoadAllEntry)
                        {
                            // Queue composite thumbnail generation if not attempted
                            if (!file.attemptedLoad && !file.textureID)
                            {
                                file.attemptedLoad = true;

                                // Collect area files for the worker thread
                                std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> areaFiles;
                                for (size_t j = 0; j < sCachedFiles.size(); j++)
                                {
                                    const auto& f = sCachedFiles[j];
                                    if (!f.isDirectory && f.extension == ".area" && !f.isLoadAllEntry)
                                    {
                                        auto fe = std::dynamic_pointer_cast<FileEntry>(f.entry);
                                        if (fe)
                                            areaFiles.push_back({f.archive, fe});
                                    }
                                }

                                if (!areaFiles.empty())
                                {
                                    ThumbnailRequest req;
                                    req.isComposite = true;
                                    req.compositeAreaFiles = std::move(areaFiles);
                                    req.fileIndex = static_cast<int>(i);
                                    req.generation = sCurrentGeneration;

                                    {
                                        std::lock_guard<std::mutex> lock(sQueueMutex);
                                        sLoadQueue.push_back(std::move(req));
                                    }
                                    sQueueCV.notify_one();
                                }
                            }

                            // Draw the thumbnail or fallback to blue box
                            if (file.textureID)
                            {
                                drawList->AddImage(file.textureID, contentMin, contentMin + ImVec2(iconSize, iconSize));
                            }
                            else
                            {
                                // Fallback: Blue gradient box with play icon
                                ImU32 bgColor = IM_COL32(40, 80, 140, 255);
                                drawList->AddRectFilled(contentMin + ImVec2(5, 5), contentMin + ImVec2(iconSize - 5, iconSize - 5), bgColor, 8.0f);
                                ImVec2 center = contentMin + ImVec2(iconSize * 0.5f, iconSize * 0.5f);
                                float triSize = iconSize * 0.25f;
                                drawList->AddTriangleFilled(
                                    center + ImVec2(-triSize * 0.5f, -triSize),
                                    center + ImVec2(-triSize * 0.5f, triSize),
                                    center + ImVec2(triSize, 0),
                                    IM_COL32(255, 255, 255, 255)
                                );
                            }

                            // Right-click context menu for Load All
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

                            if (ImGui::BeginPopupContextItem("##loadallcontext"))
                            {
                                std::string folderName = sSelectedFolder ? wstring_to_utf8(sSelectedFolder->getEntryName()) : "Unknown";

                                if (ImGui::MenuItem("View Heightmap"))
                                {
                                    Heightmap::ViewAllAreasHeightmap(state, folderName, sCurrentGeneration);
                                }

                                if (ImGui::BeginMenu("Extract Heightmap"))
                                {
                                    if (ImGui::MenuItem("PNG"))
                                    {
                                        Heightmap::SetPendingAllHeightmapExport(folderName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = folderName + "_heightmap.png";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapPngDlg", "Export Heightmap as PNG", ".png", config);
                                    }
                                    if (ImGui::MenuItem("JPEG"))
                                    {
                                        Heightmap::SetPendingAllHeightmapExport(folderName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = folderName + "_heightmap.jpg";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapJpgDlg", "Export Heightmap as JPEG", ".jpg", config);
                                    }
                                    if (ImGui::MenuItem("BMP"))
                                    {
                                        Heightmap::SetPendingAllHeightmapExport(folderName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = folderName + "_heightmap.bmp";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapBmpDlg", "Export Heightmap as BMP", ".bmp", config);
                                    }
                                    if (ImGui::MenuItem("TGA"))
                                    {
                                        Heightmap::SetPendingAllHeightmapExport(folderName);
                                        IGFD::FileDialogConfig config;
                                        config.path = ".";
                                        config.fileName = folderName + "_heightmap.tga";
                                        config.flags = ImGuiFileDialogFlags_Modal;
                                        ImGuiFileDialog::Instance()->OpenDialog("ExportHeightmapTgaDlg", "Export Heightmap as TGA", ".tga", config);
                                    }
                                    ImGui::EndMenu();
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar(2);
                        }
                        else if (file.isDirectory && sFolderIcon)
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
                        else if (file.extension == ".bnk" && sAudioIcon)
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
                            else if (file.extension == ".bnk")  bgColor = IM_COL32(90, 70, 90, 255);

                            drawList->AddRectFilled(contentMin + ImVec2(10, 10), contentMin + ImVec2(iconSize - 10, iconSize - 10), bgColor, 4.0f);
                        }

                        ImVec2 textStart = contentMin + ImVec2(0, iconSize);
                        ImGui::SetCursorScreenPos(textStart + ImVec2(2, 0));

                        std::string stem = file.name;
                        if (!file.isDirectory && !file.isLoadAllEntry)
                        {
                            size_t lastDot = stem.rfind('.');
                            if (lastDot != std::string::npos)
                                stem = stem.substr(0, lastDot);
                        }

                        ImVec4 clipRect(cellMin.x, cellMin.y, cellMax.x, textStart.y + (textLineHeight * nameLines));
                        drawList->PushClipRect(ImVec2(clipRect.x, clipRect.y), ImVec2(clipRect.z, clipRect.w), true);

                        ImVec4 nameColor(1.0f, 1.0f, 1.0f, 1.0f);
                        if (file.isLoadAllEntry)
                            nameColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
                        else if (file.extension == ".m3")
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

                        if (!file.isDirectory && !file.isLoadAllEntry)
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
                ID3D11ShaderResourceView* srv = reinterpret_cast<ID3D11ShaderResourceView*>(file.textureID);
                if (srv) srv->Release();
            }
        }

        sCachedFiles.clear();
        sNeedsRefresh = true;


        sExportArchive = nullptr;
        sExportFileEntry = nullptr;
        sExportDefaultName.clear();
        sExportInProgress = false;
        sNotificationTimer = 0.0f;

        Audio::AudioManager::Get().StopAll();
    }

    void Draw(AppState& state)
    {
        EnsureWorkerStarted();

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Space, false))
        {
            Toggle();
        }

        if (!sContentBrowserIcon)
            sContentBrowserIcon = UI_Utils::LoadTextureFromResource(IDR_ICON_CONTENTBROWSER);

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
                ImGui::Image(sContentBrowserIcon, ImVec2(iconSize, iconSize), ImVec2(0,0), ImVec2(1,1));
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


        if (ImGuiFileDialog::Instance()->Display("ExportGLBDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

                size_t extPos = fileName.rfind('.');
                std::string exportName = (extPos != std::string::npos) ? fileName.substr(0, extPos) : fileName;


                M3ModelData modelData = M3Loader::LoadFromFile(sExportArchive, sExportFileEntry);
                if (!modelData.geometry.vertices.empty())
                {
                    auto tempRender = std::make_unique<M3Render>(modelData, sExportArchive, false);

                    sExportInProgress = true;
                    sExportProgress = 0;
                    sExportTotal = 100;
                    sExportStatus = "Starting export...";

                    M3Render* renderPtr = tempRender.release();
                    ArchivePtr archiveCopy = sExportArchive;

                    std::thread([renderPtr, archiveCopy, dirPath, exportName]() {
                        M3Export::ExportSettings settings;
                        settings.outputPath = dirPath;
                        settings.customName = exportName;
                        settings.activeVariant = -1;
                        settings.exportTextures = true;
                        settings.exportAnimations = true;
                        settings.exportSkeleton = true;

                        auto result = M3Export::ExportToGLB(renderPtr, archiveCopy, settings,
                            [](int cur, int total, const std::string& status) {
                                sExportProgress = cur;
                                sExportTotal = total;
                                std::lock_guard<std::mutex> lock(sExportMutex);
                                sExportStatus = status;
                            });

                        delete renderPtr;

                        {
                            std::lock_guard<std::mutex> lock(sExportMutex);
                            sExportResult = result;
                        }
                        sExportInProgress = false;
                        sShowExportResult = true;
                    }).detach();
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to load model for export";
                    sNotificationTimer = 3.0f;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }


        if (ImGuiFileDialog::Instance()->Display("ExportFBXDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

                size_t extPos = fileName.rfind('.');
                std::string exportName = (extPos != std::string::npos) ? fileName.substr(0, extPos) : fileName;


                M3ModelData modelData = M3Loader::LoadFromFile(sExportArchive, sExportFileEntry);
                if (!modelData.geometry.vertices.empty())
                {
                    auto tempRender = std::make_unique<M3Render>(modelData, sExportArchive, false);

                    sExportInProgress = true;
                    sExportProgress = 0;
                    sExportTotal = 100;
                    sExportStatus = "Starting export...";

                    M3Render* renderPtr = tempRender.release();
                    ArchivePtr archiveCopy = sExportArchive;

                    std::thread([renderPtr, archiveCopy, dirPath, exportName]() {
                        M3Export::ExportSettings settings;
                        settings.outputPath = dirPath;
                        settings.customName = exportName;
                        settings.activeVariant = -1;
                        settings.exportTextures = true;
                        settings.exportAnimations = true;
                        settings.exportSkeleton = true;

                        auto result = M3Export::ExportToFBX(renderPtr, archiveCopy, settings,
                            [](int cur, int total, const std::string& status) {
                                sExportProgress = cur;
                                sExportTotal = total;
                                std::lock_guard<std::mutex> lock(sExportMutex);
                                sExportStatus = status;
                            });

                        delete renderPtr;

                        {
                            std::lock_guard<std::mutex> lock(sExportMutex);
                            sExportResult = result;
                        }
                        sExportInProgress = false;
                        sShowExportResult = true;
                    }).detach();
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to load model for export";
                    sNotificationTimer = 3.0f;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractM3RawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::vector<uint8_t> buffer;
                if (sExportArchive->openFileStream(sExportFileEntry, buffer))
                {
                    std::ofstream out(filePath, std::ios::binary);
                    if (out.is_open())
                    {
                        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                        out.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "M3 extracted successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to write file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to read M3 from archive";
                }
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractTexRawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::vector<uint8_t> buffer;
                if (sExportArchive->openFileStream(sExportFileEntry, buffer))
                {
                    std::ofstream out(filePath, std::ios::binary);
                    if (out.is_open())
                    {
                        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                        out.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "Texture extracted successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to write file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to read texture from archive";
                }
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportTexPNGDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                bool success = Tex::ExportTextureFromArchive(sExportArchive, sExportFileEntry, filePath, Tex::ExportFormat::PNG);
                sNotificationSuccess = success;
                sNotificationMessage = success ? "PNG exported successfully!" : "Failed to export PNG";
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportTexJPGDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                bool success = Tex::ExportTextureFromArchive(sExportArchive, sExportFileEntry, filePath, Tex::ExportFormat::JPEG);
                sNotificationSuccess = success;
                sNotificationMessage = success ? "JPG exported successfully!" : "Failed to export JPG";
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportTexTIFFDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                bool success = Tex::ExportTextureFromArchive(sExportArchive, sExportFileEntry, filePath, Tex::ExportFormat::TIFF);
                sNotificationSuccess = success;
                sNotificationMessage = success ? "TIFF exported successfully!" : "Failed to export TIFF";
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportTexDDSDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                bool success = Tex::ExportTextureFromArchive(sExportArchive, sExportFileEntry, filePath, Tex::ExportFormat::DDS);
                sNotificationSuccess = success;
                sNotificationMessage = success ? "DDS exported successfully!" : "Failed to export DDS";
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractAreaRawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::vector<uint8_t> buffer;
                if (sExportArchive->openFileStream(sExportFileEntry, buffer))
                {
                    std::ofstream out(filePath, std::ios::binary);
                    if (out.is_open())
                    {
                        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                        out.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "Area extracted successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to write file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to read area from archive";
                }
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportWsTerrainDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();

                auto areaFile = std::make_shared<AreaFile>(sExportArchive, sExportFileEntry);
                if (areaFile->load())
                {
                    TerrainExport::ExportSettings settings;
                    settings.outputPath = dirPath;
                    settings.scale = 1.0f;

                    auto result = TerrainExport::ExportAreaToTerrain(areaFile, settings);
                    sNotificationSuccess = result.success;
                    sNotificationMessage = result.success ? "Terrain exported successfully!" : ("Export failed: " + result.errorMessage);
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to load area file";
                }
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractWemRawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportArchive && sExportFileEntry)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::vector<uint8_t> buffer;
                if (sExportArchive->openFileStream(sExportFileEntry, buffer))
                {
                    std::ofstream out(filePath, std::ios::binary);
                    if (out.is_open())
                    {
                        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                        out.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "WEM extracted successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to write file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to read WEM from archive";
                }
                sNotificationTimer = 3.0f;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractBnkWemRawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && !sAudioExportData.empty())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::ofstream out(filePath, std::ios::binary);
                if (out.is_open())
                {
                    out.write(reinterpret_cast<const char*>(sAudioExportData.data()), sAudioExportData.size());
                    out.close();
                    sNotificationSuccess = true;
                    sNotificationMessage = "WEM extracted successfully!";
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to write file";
                }
                sNotificationTimer = 3.0f;
                sAudioExportData.clear();
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportAudioDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && !sAudioExportData.empty())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                Audio::AudioManager::Get().Initialize();

                bool success = Audio::AudioExport::ExportWEMToWAV(sAudioExportData.data(), sAudioExportData.size(), filePath);

                if (success)
                {
                    sNotificationSuccess = true;
                    sNotificationMessage = "WAV exported successfully!";
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = Audio::AudioExport::GetLastError();
                }
                sNotificationTimer = 3.0f;
                sAudioExportData.clear();
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractBnkDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && !sBnkExportData.empty())
            {
                std::string folderPath = ImGuiFileDialog::Instance()->GetCurrentPath();

                int extractedCount = 0;
                std::string errorMsg;

                const uint8_t* data = sBnkExportData.data();
                size_t size = sBnkExportData.size();
                size_t pos = 0;

                std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> wemEntries;
                const uint8_t* dataSection = nullptr;
                size_t dataSectionSize = 0;

                while (pos + 8 <= size) {
                    char sectionId[5] = {0};
                    memcpy(sectionId, data + pos, 4);
                    uint32_t sectionSize = *(uint32_t*)(data + pos + 4);

                    if (memcmp(sectionId, "DIDX", 4) == 0) {
                        const uint8_t* didx = data + pos + 8;
                        size_t numEntries = sectionSize / 12;
                        for (size_t i = 0; i < numEntries; i++) {
                            uint32_t wemId = *(uint32_t*)(didx + i * 12);
                            uint32_t wemOffset = *(uint32_t*)(didx + i * 12 + 4);
                            uint32_t wemSize = *(uint32_t*)(didx + i * 12 + 8);
                            wemEntries.push_back({wemId, wemOffset, wemSize});
                        }
                    } else if (memcmp(sectionId, "DATA", 4) == 0) {
                        dataSection = data + pos + 8;
                        dataSectionSize = sectionSize;
                    }

                    pos += 8 + sectionSize;
                }

                if (dataSection && !wemEntries.empty()) {
                    for (const auto& [wemId, wemOffset, wemSize] : wemEntries) {
                        if (wemOffset + wemSize <= dataSectionSize) {
                            std::string displayName = GetWemDisplayName(wemId);
                            std::string wemPath = folderPath + "/" + displayName + ".wem";
                            std::ofstream outFile(wemPath, std::ios::binary);
                            if (outFile.is_open()) {
                                outFile.write((const char*)(dataSection + wemOffset), wemSize);
                                outFile.close();
                                extractedCount++;
                            }
                        }
                    }

                    sNotificationSuccess = true;
                    sNotificationMessage = "Extracted " + std::to_string(extractedCount) + " WEM files";
                } else {
                    sNotificationSuccess = false;
                    sNotificationMessage = "No WEM data found in BNK";
                }

                sNotificationTimer = 3.0f;
                sBnkExportData.clear();
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractBnkRawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportFileEntry && sExportArchive)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                std::vector<uint8_t> data;
                if (sExportArchive->getFileData(sExportFileEntry, data) && !data.empty())
                {
                    std::ofstream outFile(filePath, std::ios::binary);
                    if (outFile.is_open())
                    {
                        outFile.write((const char*)data.data(), data.size());
                        outFile.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "BNK extracted successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to create output file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to read BNK data";
                }
                sNotificationTimer = 3.0f;
                sExportFileEntry = nullptr;
                sExportArchive = nullptr;
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportTblCsvDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && !sTblExportData.empty())
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                Tbl::File tblFile;
                if (tblFile.load(sTblExportData.data(), sTblExportData.size()))
                {
                    std::wofstream out(filePath);
                    if (out.is_open())
                    {
                        const auto& cols = tblFile.getColumns();
                        for (size_t i = 0; i < cols.size(); ++i)
                        {
                            if (i > 0) out << L";";
                            out << L"\"" << cols[i].name << L"\"";
                        }
                        out << std::endl;

                        for (uint32_t row = 0; row < tblFile.getRecordCount(); ++row)
                        {
                            for (size_t col = 0; col < cols.size(); ++col)
                            {
                                if (col > 0) out << L";";

                                switch (cols[col].dataType)
                                {
                                case Tbl::DataType::Uint:
                                case Tbl::DataType::Flags:
                                    out << tblFile.getUint(row, col);
                                    break;
                                case Tbl::DataType::Float:
                                    out << tblFile.getFloat(row, col);
                                    break;
                                case Tbl::DataType::Ulong:
                                    out << tblFile.getInt64(row, col);
                                    break;
                                case Tbl::DataType::String:
                                {
                                    std::wstring str = tblFile.getString(row, col);
                                    std::wstring escaped;
                                    for (wchar_t c : str)
                                    {
                                        if (c == L'"') escaped += L"\"\"";
                                        else escaped += c;
                                    }
                                    out << L"\"" << escaped << L"\"";
                                    break;
                                }
                                default:
                                    out << tblFile.getUint(row, col);
                                    break;
                                }
                            }
                            out << std::endl;
                        }
                        out.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "CSV exported successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to create output file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to parse TBL file";
                }
                sNotificationTimer = 3.0f;
                sTblExportData.clear();
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExtractTblRawDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && sExportFileEntry && sExportArchive)
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                std::vector<uint8_t> data;
                if (sExportArchive->getFileData(sExportFileEntry, data) && !data.empty())
                {
                    std::ofstream outFile(filePath, std::ios::binary);
                    if (outFile.is_open())
                    {
                        outFile.write((const char*)data.data(), data.size());
                        outFile.close();
                        sNotificationSuccess = true;
                        sNotificationMessage = "TBL extracted successfully!";
                    }
                    else
                    {
                        sNotificationSuccess = false;
                        sNotificationMessage = "Failed to create output file";
                    }
                }
                else
                {
                    sNotificationSuccess = false;
                    sNotificationMessage = "Failed to read TBL data";
                }
                sNotificationTimer = 3.0f;
                sExportFileEntry = nullptr;
                sExportArchive = nullptr;
            }
            ImGuiFileDialog::Instance()->Close();
        }


        if (sShowExportResult)
        {
            M3Export::ExportResult result;
            {
                std::lock_guard<std::mutex> lock(sExportMutex);
                result = sExportResult;
            }
            sNotificationSuccess = result.success;
            sNotificationMessage = result.success ? "Export successful!" : ("Export failed: " + result.errorMessage);
            sNotificationTimer = 3.0f;
            sShowExportResult = false;
        }


        if (sNotificationTimer > 0.0f)
        {
            sNotificationTimer -= ImGui::GetIO().DeltaTime;
            float alpha = std::min(1.0f, sNotificationTimer);

            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + 60.0f);

            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.85f * alpha);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

            ImGuiWindowFlags notifyFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize;

            if (ImGui::Begin("##ExportNotification", nullptr, notifyFlags))
            {
                if (sNotificationSuccess)
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", sNotificationMessage.c_str());
                else
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", sNotificationMessage.c_str());
            }
            ImGui::End();

            ImGui::PopStyleVar(2);
        }


        if (sExportInProgress.load())
        {
            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);

            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowBgAlpha(0.95f);

            ImGuiWindowFlags progressFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

            if (ImGui::Begin("##ExportProgress", nullptr, progressFlags))
            {
                std::string status;
                {
                    std::lock_guard<std::mutex> lock(sExportMutex);
                    status = sExportStatus;
                }
                ImGui::Text("Exporting...");
                ImGui::Text("%s", status.c_str());
                float progress = sExportTotal > 0 ? (float)sExportProgress.load() / (float)sExportTotal : 0.0f;
                ImGui::ProgressBar(progress, ImVec2(300, 20));
            }
            ImGui::End();
        }

        Heightmap::DrawHeightmapViewer(sCurrentGeneration);
    }

    void NavigateToPath(AppState& state, const std::string& folderPath)
    {
        if (folderPath.empty()) return;

        std::vector<std::string> pathParts = SplitPath(folderPath);
        if (pathParts.empty()) return;

        for (const auto& archive : state.archives)
        {
            if (!archive) continue;
            auto root = archive->getRoot();
            if (!root) continue;

            IFileSystemEntryPtr found = FindFolderByPath(root, pathParts, 0);
            if (found)
            {
                sSelectedFolder = found;
                sSelectedArchive = archive;
                sSearchFilter[0] = '\0';
                sRequestTreeSync = true;
                sNeedsRefresh = true;

                if (!sIsOpen)
                {
                    sIsOpen = true;
                }

                return;
            }
        }

        std::string justFolder = ExtractFolderPath(folderPath);
        if (!justFolder.empty() && justFolder != folderPath)
        {
            NavigateToPath(state, justFolder);
        }
    }
}