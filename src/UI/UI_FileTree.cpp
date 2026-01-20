#include "UI_FileTree.h"
#include "UI_Globals.h"
#include "UI_Utils.h"
#include "UI_Tables.h"

#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include "../tex/tex.h"

#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

#include <imgui.h>

extern void SnapCameraToLoaded(AppState& state);
extern void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax);

static void LoadAreasInFolder(AppState& state, const ArchivePtr& arc, const IFileSystemEntryPtr& folderEntry)
{
    if (!arc || !folderEntry || !folderEntry->isDirectory()) return;

    ResetAreaReferencePosition();

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;

    for (const auto& child : folderEntry->getChildren())
    {
        if (!child || child->isDirectory()) continue;

        const std::string childName = wstring_to_utf8(child->getEntryName());
        if (!EndsWithNoCase(childName, ".area")) continue;

        const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(child);
        if (!fileEntry) continue;

        const auto af = std::make_shared<AreaFile>(arc, fileEntry);
        if (af->load())
        {
            af->loadAllProps();
            gLoadedAreas.push_back(af);
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

    ResetAreaReferencePosition();

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;
    gLoadedModel = nullptr;

    auto af = std::make_shared<AreaFile>(arc, fileEntry);
    if (af->load())
    {
        af->loadAllProps();
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

        ImGui::PushID(static_cast<const void*>(entry.get()));

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
                RenderEntryRecursive_Impl(state, folder, entry, currentArc, max_width, depth + 1.0f);

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

                    ImGui::PushID(static_cast<const void*>(f.get()));

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
                        else if (EndsWithNoCase(fname, ".tbl"))
                        {
                            if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f); fileEntry && currentArc)
                                UI_Tables::OpenTblFile(state, currentArc, fileEntry);
                        }
                    }

                    if (ImGui::BeginPopupContextItem("FileContextMenu"))
                    {
                        if (ImGui::MenuItem("Extract..."))
                        {
                            if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f); fileEntry && currentArc)
                            {
                                gExtractContext.showDialog = true;
                                gExtractContext.isFolder = false;
                                gExtractContext.arc = currentArc;
                                gExtractContext.file = fileEntry;
                                gExtractContext.folder = nullptr;
                                gExtractContext.itemName = fname;
                            }
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::BeginPopupContextItem("FolderContextMenu"))
        {
            if (ImGui::MenuItem("Extract Folder..."))
            {
                gExtractContext.showDialog = true;
                gExtractContext.isFolder = true;
                gExtractContext.arc = currentArc;
                gExtractContext.file = nullptr;
                gExtractContext.folder = entry;
                gExtractContext.itemName = name;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    else
    {
        const std::string name = wstring_to_utf8(entry->getEntryName());
        const float indent_px = depth * ImGui::GetStyle().IndentSpacing;
        const float text_w = ImGui::CalcTextSize(name.c_str()).x;
        const float current_w = indent_px + text_w + 50.0f;
        if (current_w > max_width) max_width = current_w;

        ImGui::PushID(static_cast<const void*>(entry.get()));

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
            else if (EndsWithNoCase(name, ".tbl"))
            {
                if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry); fileEntry && currentArc)
                    UI_Tables::OpenTblFile(state, currentArc, fileEntry);
            }
        }

        if (ImGui::BeginPopupContextItem("FileContextMenu"))
        {
            if (ImGui::MenuItem("Extract..."))
            {
                if (const auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry); fileEntry && currentArc)
                {
                    gExtractContext.showDialog = true;
                    gExtractContext.isFolder = false;
                    gExtractContext.arc = currentArc;
                    gExtractContext.file = fileEntry;
                    gExtractContext.folder = nullptr;
                    gExtractContext.itemName = name;
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
}

void UI_RenderFileTab(AppState& state, float& outContentWidth)
{
    ImGui::Text("EXPLORER");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 5));

    if (ImGui::Button("Dump All", ImVec2(-1, 0)))
    {
        gShowDumpFolderDialog = true;
    }

    ImGui::Dummy(ImVec2(0, 5));

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
        outContentWidth = calculated_width;
    }
}