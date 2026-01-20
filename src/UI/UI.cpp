#include "UI.h"
#include "UI_Globals.h"
#include "UI_FileTree.h"
#include "UI_AreaTab.h"
#include "UI_ModelTab.h"
#include "UI_Tables.h"
#include "UI_AreaInfo.h"
#include "UI_RenderWorld.h"
#include "UI_ChunkTextures.h"

#include "../About/about.h"
#include "../settings/Settings.h"
#include "UI_Models.h"
#include "splashscreen.h"
#include "../tex/tex.h"

#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

extern void PushSplashButtonColors();
extern void PopSplashButtonColors();
extern bool gAreaIconLoaded;
extern unsigned int gAreaIconTexture;
extern bool gCharacterIconLoaded;
extern unsigned int gCharacterIconTexture;

static void RenderLoadingOverlay()
{
    if (gIsLoadingAreas)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowBgAlpha(0.7f);

        ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                         ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("##LoadingOverlay", nullptr, overlayFlags);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

        char loadingText[512];
        snprintf(loadingText, sizeof(loadingText), "Loading %s...", gLoadingAreasName.c_str());
        float textWidth = ImGui::CalcTextSize(loadingText).x;

        char countText[64];
        snprintf(countText, sizeof(countText), "%d / %d areas", gLoadingAreasCurrent, gLoadingAreasTotal);
        float countWidth = ImGui::CalcTextSize(countText).x;

        float contentWidth = std::max(textWidth, countWidth);
        float minWidth = 300.0f;
        float maxWidth = viewport->Size.x * 0.8f;
        float windowWidth = std::clamp(contentWidth + 60.0f, minWidth, maxWidth);

        ImVec2 windowPos(
            viewport->Pos.x + (viewport->Size.x - windowWidth) * 0.5f,
            viewport->Pos.y + (viewport->Size.y - 120.0f) * 0.5f
        );

        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, 0), ImVec2(maxWidth, FLT_MAX));

        ImGuiWindowFlags loadingFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("##LoadingWindow", nullptr, loadingFlags))
        {
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
            ImGui::TextUnformatted(loadingText);
            ImGui::PopTextWrapPos();

            ImGui::Spacing();

            float progress = gLoadingAreasTotal > 0
                ? static_cast<float>(gLoadingAreasCurrent) / static_cast<float>(gLoadingAreasTotal)
                : 0.0f;

            ImGui::ProgressBar(progress, ImVec2(std::max(windowWidth - 40.0f, 260.0f), 20));

            ImGui::Spacing();
            ImGui::TextUnformatted(countText);
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    if (gIsLoadingModel)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        float viewportMin = std::min(viewport->Size.x, viewport->Size.y);
        float spinnerRadius = viewportMin * 0.08f;
        float thickness = spinnerRadius * 0.2f;

        ImVec2 center(
            viewport->Pos.x + viewport->Size.x * 0.5f,
            viewport->Pos.y + viewport->Size.y * 0.5f
        );

        static float angle = 0.0f;
        angle += ImGui::GetIO().DeltaTime * 5.0f;

        int numSegments = 30;
        float startAngle = angle;
        float arcLength = 3.14159f * 1.5f;

        for (int i = 0; i < numSegments; ++i)
        {
            float a1 = startAngle + (arcLength * i / numSegments);
            float a2 = startAngle + (arcLength * (i + 1) / numSegments);

            ImVec2 p1(center.x + std::cos(a1) * spinnerRadius, center.y + std::sin(a1) * spinnerRadius);
            ImVec2 p2(center.x + std::cos(a2) * spinnerRadius, center.y + std::sin(a2) * spinnerRadius);

            float alpha = (float)i / numSegments;
            ImU32 segColor = IM_COL32(100, 180, 255, (int)(255 * alpha));
            drawList->AddLine(p1, p2, segColor, thickness);
        }
    }
}

static void RenderDumpFolderDialog(AppState& state)
{
    if (gShowDumpFolderDialog)
    {
        IGFD::FileDialogConfig config;
        config.path = ".";
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("DumpFolderDlg", "Select Output Folder", nullptr, config);
        gShowDumpFolderDialog = false;
    }

    if (ImGuiFileDialog::Instance()->Display("DumpFolderDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string outputPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            StartDumpAll(state.archives, outputPath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

static void RenderExtractDialog(AppState& state)
{
    if (gExtractContext.showDialog)
    {
        IGFD::FileDialogConfig config;
        config.path = ".";
        config.flags = ImGuiFileDialogFlags_Modal;

        std::string title = gExtractContext.isFolder
            ? "Extract Folder: " + gExtractContext.itemName
            : "Extract File: " + gExtractContext.itemName;

        ImGuiFileDialog::Instance()->OpenDialog("ExtractDlg", title.c_str(), nullptr, config);
        gExtractContext.showDialog = false;
    }

    if (ImGuiFileDialog::Instance()->Display("ExtractDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string outputPath = ImGuiFileDialog::Instance()->GetCurrentPath();

            if (gExtractContext.isFolder && gExtractContext.folder && gExtractContext.arc)
            {
                StartExtractFolder(gExtractContext.arc, gExtractContext.folder, outputPath);
            }
            else if (!gExtractContext.isFolder && gExtractContext.file && gExtractContext.arc)
            {
                StartExtractSingle(gExtractContext.arc, gExtractContext.file, outputPath);
            }
        }

        gExtractContext.arc = nullptr;
        gExtractContext.file = nullptr;
        gExtractContext.folder = nullptr;
        gExtractContext.itemName.clear();

        ImGuiFileDialog::Instance()->Close();
    }
}

static void RenderDumpOverlay()
{
    if (!gIsDumping) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowBgAlpha(0.7f);

    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##DumpOverlay", nullptr, overlayFlags);
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    float minWidth = 400.0f;
    float maxWidth = viewport->Size.x * 0.8f;
    float windowWidth = std::clamp(500.0f, minWidth, maxWidth);

    ImVec2 windowPos(
        viewport->Pos.x + (viewport->Size.x - windowWidth) * 0.5f,
        viewport->Pos.y + (viewport->Size.y - 140.0f) * 0.5f
    );

    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, 0), ImVec2(maxWidth, FLT_MAX));

    ImGuiWindowFlags dumpFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##DumpWindow", nullptr, dumpFlags))
    {
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads < 2) numThreads = 2;
        if (numThreads > 16) numThreads = 16;

        ImGui::Text("Dumping files... (%u threads)", numThreads);
        ImGui::Spacing();

        int current = gDumpCurrent.load();
        float progress = gDumpTotal > 0
            ? static_cast<float>(current) / static_cast<float>(gDumpTotal)
            : 0.0f;

        ImGui::ProgressBar(progress, ImVec2(windowWidth - 40.0f, 20));

        ImGui::Spacing();

        char countText[64];
        snprintf(countText, sizeof(countText), "%d / %d files", current, gDumpTotal);
        ImGui::TextUnformatted(countText);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
}

void RenderUI(AppState& state)
{
    ProcessAreaLoading(state);
    ProcessModelLoading(state);
    ProcessDumping();
    HandleChunkPicking(state);

    if (!state.archivesLoaded)
    {
        PushSplashButtonColors();
        RenderSplashScreen(state);
        PopSplashButtonColors();
        return;
    }

    UI_EnsureMergedM3List(state);

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
        ImGui::Text("Camera Speed: %.1f", state.camera.MovementSpeed);
    ImGui::End();

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
        float button_height_local = 50.0f;
        float settings_y = ImGui::GetWindowHeight() - button_height_local - bottom_margin;
        float about_y = settings_y - button_height_local;

        ImGui::SetCursorPosY(about_y);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (state.aboutIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height_local - icon_size) * 0.5f;
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
            if (ImGui::Button("About", ImVec2(strip_width, button_height_local)))
                state.show_about_window = !state.show_about_window;
        }
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(settings_y);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (state.settingsIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height_local - icon_size) * 0.5f;
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
            if (ImGui::Button("Set", ImVec2(strip_width, button_height_local)))
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

            float calculated_width = 280.0f;
            if (state.active_tab_index == 0) UI_RenderFileTab(state, calculated_width);
            else if (state.active_tab_index == 1) UI_RenderAreaTab(state, calculated_width);
            else if (state.active_tab_index == 2) UI_RenderModelTab(state, calculated_width);
            state.contentWidth = calculated_width;
        }
        ImGui::End();
    }

    RenderSettingsWindow(&state.show_settings_window);
    RenderAboutWindow(&state.show_about_window);

    if (state.texPreview)
        Tex::RenderTexPreviewWindow(*state.texPreview);

    if (state.show_models_window && state.m3Render)
        UI_Models::Draw(state);

    UI_Tables::Draw(state);

    UI_AreaInfo::Draw(state);

    UI_ChunkTextures::Draw(state);

    RenderDumpFolderDialog(state);
    RenderExtractDialog(state);
    RenderDumpOverlay();
    RenderLoadingOverlay();
}