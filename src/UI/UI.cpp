#include "UI.h"
#include "UI_Globals.h"
#include "UI_Tables.h"
#include "UI_RenderWorld.h"
#include "UI_Outliner.h"
#include "UI_Details.h"
#include "UI_ContentBrowser.h"
#include "UI_TopBar.h"
#include "Settings.h"
#include "about.h"
#include "AddonManager.h"

#include "splashscreen.h"
#include "FileOps.h"
#include "../tex/tex.h"
#include "../Audio/AudioPlayerWidget.h"

#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

bool gShowSettings = false;
bool gShowAbout = false;
bool gRequestQuit = false;
static float sAddonNotificationTimer = 0.0f;
static bool sAddonNotificationSuccess = false;
static std::string sAddonNotificationMessage;

bool ShouldQuit()
{
    return gRequestQuit;
}

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

    float windowWidth = 400.0f;
    ImVec2 windowPos(
        viewport->Pos.x + (viewport->Size.x - windowWidth) * 0.5f,
        viewport->Pos.y + (viewport->Size.y - 120.0f) * 0.5f
    );

    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, 0));

    ImGuiWindowFlags loadingFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##DumpWindow", nullptr, loadingFlags))
    {
        ImGui::Text("Dumping files...");

        ImGui::Spacing();

        int current = gDumpCurrent.load();
        float progress = gDumpTotal > 0 ? static_cast<float>(current) / static_cast<float>(gDumpTotal) : 0.0f;

        char progressText[64];
        snprintf(progressText, sizeof(progressText), "%d / %d", current, gDumpTotal);

        ImGui::ProgressBar(progress, ImVec2(windowWidth - 40.0f, 20), progressText);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
}

void RenderUI(AppState& state)
{
    ProcessModelLoading(state);
    ProcessAreaLoading(state);
    ProcessDumping();

    if (state.audioInitRequested && !state.audioInitComplete)
    {
        UI_ContentBrowser::InitializeAudioDatabase(state);
    }

    if (!state.archivesLoaded)
    {
        RenderSplashScreen(state);
        return;
    }

    UI_TopBar::Draw(state, &gShowSettings, &gShowAbout, &gRequestQuit);

    UI_Outliner::Draw(state);
    UI_Details::Draw(state);
    UI_ContentBrowser::Draw(state);

    if (state.texPreview)
        Tex::RenderTexPreviewWindow(*state.texPreview);

    UI_Tables::Draw(state);
    Audio::AudioPlayerWidget::Get().Render();

    RenderDumpFolderDialog(state);
    RenderExtractDialog(state);
    RenderDumpOverlay();
    RenderLoadingOverlay();

    if (gShowSettings)
    {
        RenderSettingsWindow(&gShowSettings);
    }

    if (gShowAbout)
    {
        RenderAboutWindow(&gShowAbout);
    }

    if (UI_TopBar::ShouldShowAddonSaveDialog())
    {
        IGFD::FileDialogConfig config;
        config.path = ".";
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog("AddonSaveDlg", "Select Download Location", nullptr, config);
    }

    if (ImGuiFileDialog::Instance()->Display("AddonSaveDlg", ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string outputPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            bool success = AddonManager::SaveBlenderTerrainAddon(outputPath);
            sAddonNotificationSuccess = success;
            sAddonNotificationMessage = success ? "Addon saved successfully!" : "Failed to save addon";
            sAddonNotificationTimer = 3.0f;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (sAddonNotificationTimer > 0.0f)
    {
        sAddonNotificationTimer -= ImGui::GetIO().DeltaTime;
        float alpha = std::min(1.0f, sAddonNotificationTimer);

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 center(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + 80.0f);

        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.85f * alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

        ImGuiWindowFlags notifyFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("##AddonNotification", nullptr, notifyFlags))
        {
            if (sAddonNotificationSuccess)
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", sAddonNotificationMessage.c_str());
            else
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", sAddonNotificationMessage.c_str());
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
    }
}