#include "UI.h"
#include "UI_Globals.h"
#include "UI_FileTree.h"
#include "UI_AreaTab.h"
#include "UI_ModelTab.h"
#include "UI_Tables.h"

#include "../About/about.h"
#include "../settings/Settings.h"
#include "UI_Models.h"
#include "splashscreen.h"
#include "../tex/tex.h"

#include <imgui.h>
#include <algorithm>
#include <cfloat>

extern void PushSplashButtonColors();
extern void PopSplashButtonColors();
extern bool gAreaIconLoaded;
extern unsigned int gAreaIconTexture;
extern bool gCharacterIconLoaded;
extern unsigned int gCharacterIconTexture;

static void RenderLoadingOverlay()
{
    if (!gIsLoadingAreas) return;

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

void RenderUI(AppState& state)
{
    ProcessAreaLoading(state);

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
            if (gSelectedAreaIndex >= 0 && gSelectedAreaIndex < static_cast<int>(gLoadedAreas.size()))
            {
                if (ImGui::Button("Rotate Area 90"))
                    gLoadedAreas[gSelectedAreaIndex]->rotate90();
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
                else { state.active_tab_index == 2; state.sidebar_visible = true; }
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

    RenderLoadingOverlay();
}