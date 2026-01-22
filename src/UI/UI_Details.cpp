#include "UI_Details.h"
#include "UI_Globals.h"
#include "UI_Outliner.h"
#include "UI_ContentBrowser.h"
#include "UI_Utils.h"
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"
#include "../models/M3Common.h"
#include "../export/M3Export.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>

namespace UI_Details
{
    static std::atomic<bool> sExportInProgress{false};
    static std::atomic<int> sExportProgress{0};
    static std::atomic<int> sExportTotal{100};
    static std::string sExportStatus;
    static std::mutex sExportMutex;
    static M3Export::ExportResult sExportResult;
    static bool sShowExportResult = false;
    static float sNotificationTimer = 0.0f;
    static std::string sNotificationMessage;
    static bool sNotificationSuccess = true;

    void Reset()
    {
        sExportInProgress = false;
        sNotificationTimer = 0.0f;
    }

    static void DrawM3Details(AppState& state)
    {
        M3Render* render = state.m3Render.get();
        if (!render) return;

        size_t submeshCount = render->getSubmeshCount();
        size_t materialCount = render->getMaterialCount();
        size_t boneCount = render->getBoneCount();
        size_t animCount = render->getAnimationCount();
        size_t texCount = render->getAllTextures().size();

        ImGui::Text("Submeshes: %zu", submeshCount);
        ImGui::Text("Materials: %zu", materialCount);
        ImGui::Text("Bones: %zu", boneCount);
        ImGui::Text("Animations: %zu", animCount);
        ImGui::Text("Textures: %zu", texCount);

        ImGui::Separator();

        bool showSkel = render->getShowSkeleton();
        if (ImGui::Checkbox("Show Skeleton", &showSkel))
            render->setShowSkeleton(showSkel);

        std::set<uint8_t> uniqueGroups;
        for (size_t i = 0; i < submeshCount; ++i)
        {
            uint8_t gid = render->getSubmesh(i).groupId;
            if (gid != 255) uniqueGroups.insert(gid);
        }

        if (uniqueGroups.size() > 1)
        {
            ImGui::Separator();

            int currentVariant = render->getActiveVariant();
            char preview[32];
            if (currentVariant == -1)
                snprintf(preview, sizeof(preview), "All");
            else
                snprintf(preview, sizeof(preview), "%d", currentVariant);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Variant:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);

            if (ImGui::BeginCombo("##VariantCombo", preview))
            {
                bool selAll = (currentVariant == -1);
                if (ImGui::Selectable("All", selAll))
                {
                    render->setActiveVariant(-1);
                    for (size_t i = 0; i < submeshCount; ++i)
                        render->setSubmeshVisible(i, true);
                }

                for (uint8_t gid : uniqueGroups)
                {
                    bool sel = (currentVariant == (int)gid);
                    char label[32];
                    snprintf(label, sizeof(label), "%d", gid);
                    if (ImGui::Selectable(label, sel))
                        render->setActiveVariant((int)gid);
                }

                ImGui::EndCombo();
            }
        }

        if (animCount > 0 && ImGui::CollapsingHeader("Animations"))
        {
            const auto& anims = render->getAllAnimations();
            int playingIdx = render->getPlayingAnimation();

            for (size_t i = 0; i < anims.size(); ++i)
            {
                const auto& anim = anims[i];
                bool isPlaying = (playingIdx == (int)i);
                float duration = (anim.timestampEnd - anim.timestampStart) / 1000.0f;

                char label[64];
                snprintf(label, sizeof(label), "%s Anim %zu (%.2fs)###anim%zu", isPlaying ? ">" : " ", i, duration, i);

                if (isPlaying)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

                if (ImGui::Selectable(label, isPlaying))
                {
                    if (isPlaying)
                        render->stopAnimation();
                    else
                        render->playAnimation((int)i);
                }

                if (isPlaying)
                    ImGui::PopStyleColor();
            }
        }

        ImGui::Separator();

        if (!sExportInProgress.load())
        {
            const float buttonHeight = 24.0f;
            const float fontHeight = ImGui::GetFontSize();
            const float padY = (buttonHeight - fontHeight) * 0.5f;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, padY));

            if (ImGui::Button("Export GLB", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight)))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_Modal;
                std::string defaultName = render->getModelName();
                if (defaultName.size() > 3 && defaultName.substr(defaultName.size() - 3) == ".m3")
                    defaultName = defaultName.substr(0, defaultName.size() - 3);
                defaultName += ".glb";
                ImGuiFileDialog::Instance()->OpenDialog("ExportGLBDlg", "Export GLB", ".glb", config);
            }

            if (ImGui::Button("Export FBX", ImVec2(ImGui::GetContentRegionAvail().x, buttonHeight)))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_Modal;
                std::string defaultName = render->getModelName();
                if (defaultName.size() > 3 && defaultName.substr(defaultName.size() - 3) == ".m3")
                    defaultName = defaultName.substr(0, defaultName.size() - 3);
                defaultName += ".fbx";
                ImGuiFileDialog::Instance()->OpenDialog("ExportFBXDlg", "Export FBX", ".fbx", config);
            }

            ImGui::PopStyleVar();
        }
        else
        {
            std::string status;
            {
                std::lock_guard<std::mutex> lock(sExportMutex);
                status = sExportStatus;
            }
            ImGui::Text("%s", status.c_str());
            float progress = sExportTotal > 0 ? (float)sExportProgress.load() / (float)sExportTotal : 0.0f;
            ImGui::ProgressBar(progress, ImVec2(-1, 16));
        }
    }

    static void DrawAreaSelectionDetails(AppState& state)
    {
        if (gSelectedAreaIndex < 0 || gSelectedAreaIndex >= static_cast<int>(gLoadedAreas.size()))
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select an item in the Outliner");
            return;
        }

        const auto& area = gLoadedAreas[gSelectedAreaIndex];
        if (!area)
        {
            ImGui::Text("Invalid area");
            return;
        }

        ImGui::Text("Tile: %d, %d", area->getTileX(), area->getTileY());

        glm::vec3 offset = area->getWorldOffset();
        ImGui::Text("World Offset: %.1f, %.1f, %.1f", offset.x, offset.y, offset.z);

        glm::vec3 minB = area->getMinBounds();
        glm::vec3 maxB = area->getMaxBounds();
        ImGui::Text("Bounds Min: %.1f, %.1f, %.1f", minB.x, minB.y, minB.z);
        ImGui::Text("Bounds Max: %.1f, %.1f, %.1f", maxB.x, maxB.y, maxB.z);

        const auto& chunks = area->getChunks();
        ImGui::Text("Chunks: %zu", chunks.size());

        size_t propCount = area->getPropCount();
        size_t loadedProps = area->getLoadedPropCount();
        ImGui::Text("Props: %zu / %zu loaded", loadedProps, propCount);

        if (loadedProps < propCount)
        {
            if (ImGui::Button("Load Props", ImVec2(-1, 24)))
            {
                area->loadAllProps();
            }
        }
    }

    void Draw(AppState& state)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float sidebarWidth = UI_Outliner::GetSidebarWidth();
        float outlinerHeight = UI_Outliner::GetWindowHeight();
        float contentBrowserHeight = UI_ContentBrowser::GetHeight();

        float topY = viewport->Pos.y + outlinerHeight;
        float availableHeight = viewport->Size.y - outlinerHeight - contentBrowserHeight;
        float startX = viewport->Pos.x + viewport->Size.x - sidebarWidth;

        ImGui::SetNextWindowPos(ImVec2(startX, topY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth, availableHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.95f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        if (ImGui::Begin("Details", nullptr, flags))
        {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(startX, topY));

            ImGui::InvisibleButton("##DetailsResizeW", ImVec2(5.0f, availableHeight));
            if (ImGui::IsItemActive())
            {
                float newWidth = sidebarWidth - ImGui::GetIO().MouseDelta.x;
                UI_Outliner::SetSidebarWidth(newWidth);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            ImGui::SetCursorScreenPos(cursorPos);

            ImGui::BeginChild("##DetailsScroll", ImVec2(0, 0), false);

            if (gLoadedModel)
            {
                DrawM3Details(state);
            }
            else if (!gLoadedAreas.empty())
            {
                DrawAreaSelectionDetails(state);
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No content loaded");
                ImGui::Spacing();
                ImGui::TextWrapped("Use the Content Browser to load .area or .m3 files.");
            }

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();

        if (ImGuiFileDialog::Instance()->Display("ExportGLBDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && state.m3Render)
            {
                std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

                size_t extPos = fileName.rfind('.');
                std::string exportName = (extPos != std::string::npos) ? fileName.substr(0, extPos) : fileName;

                M3Render* render = state.m3Render.get();
                sExportInProgress = true;
                sExportProgress = 0;
                sExportTotal = 100;
                sExportStatus = "Starting export...";

                std::thread([render, dirPath, exportName]() {
                    M3Export::ExportSettings settings;
                    settings.outputPath = dirPath;
                    settings.customName = exportName;
                    settings.activeVariant = render->getActiveVariant();
                    settings.exportTextures = true;
                    settings.exportAnimations = true;
                    settings.exportSkeleton = true;

                    auto result = M3Export::ExportToGLB(render, gPendingModelArchive, settings,
                        [](int cur, int total, const std::string& status) {
                            sExportProgress = cur;
                            sExportTotal = total;
                            std::lock_guard<std::mutex> lock(sExportMutex);
                            sExportStatus = status;
                        });

                    {
                        std::lock_guard<std::mutex> lock(sExportMutex);
                        sExportResult = result;
                    }
                    sExportInProgress = false;
                    sShowExportResult = true;
                }).detach();
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (ImGuiFileDialog::Instance()->Display("ExportFBXDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
        {
            if (ImGuiFileDialog::Instance()->IsOk() && state.m3Render)
            {
                std::string dirPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

                size_t extPos = fileName.rfind('.');
                std::string exportName = (extPos != std::string::npos) ? fileName.substr(0, extPos) : fileName;

                M3Render* render = state.m3Render.get();
                sExportInProgress = true;
                sExportProgress = 0;
                sExportTotal = 100;
                sExportStatus = "Starting export...";

                std::thread([render, dirPath, exportName]() {
                    M3Export::ExportSettings settings;
                    settings.outputPath = dirPath;
                    settings.customName = exportName;
                    settings.activeVariant = render->getActiveVariant();
                    settings.exportTextures = true;
                    settings.exportAnimations = true;
                    settings.exportSkeleton = true;

                    auto result = M3Export::ExportToFBX(render, gPendingModelArchive, settings,
                        [](int cur, int total, const std::string& status) {
                            sExportProgress = cur;
                            sExportTotal = total;
                            std::lock_guard<std::mutex> lock(sExportMutex);
                            sExportStatus = status;
                        });

                    {
                        std::lock_guard<std::mutex> lock(sExportMutex);
                        sExportResult = result;
                    }
                    sExportInProgress = false;
                    sShowExportResult = true;
                }).detach();
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

            ImVec2 center(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + 60.0f);

            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.85f * alpha);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

            ImGuiWindowFlags notifyFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize;

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
    }
}