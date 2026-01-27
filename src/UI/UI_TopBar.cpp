#include "UI_TopBar.h"
#include <imgui.h>
#include <cstdio>

namespace UI_TopBar
{
    static const float MENUBAR_HEIGHT = 20.0f;
    static const float TOOLBAR_HEIGHT = 32.0f;
    static bool sShowSpeedPopup = false;
    static bool sSpeedPopupJustOpened = false;

    float GetHeight()
    {
        return MENUBAR_HEIGHT + TOOLBAR_HEIGHT;
    }

    void Draw(AppState& state, bool* showSettings, bool* showAbout, bool* requestQuit)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, MENUBAR_HEIGHT));

        ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_MenuBar;

        if (ImGui::Begin("##MenuBar", nullptr, menuFlags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Settings"))
                    {
                        *showSettings = true;
                    }
                    if (ImGui::MenuItem("About"))
                    {
                        *showAbout = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit", "Alt+F4"))
                    {
                        *requestQuit = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
        }
        ImGui::End();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + MENUBAR_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, TOOLBAR_HEIGHT));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.06f, 1.0f));

        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

        if (ImGui::Begin("##Toolbar", nullptr, toolbarFlags))
        {
            float rightPadding = 12.0f;
            char speedText[64];
            snprintf(speedText, sizeof(speedText), "Speed: %.1f", state.camera.MovementSpeed);
            float textWidth = ImGui::CalcTextSize(speedText).x;
            float buttonWidth = textWidth + 16.0f;
            float buttonHeight = 22.0f;

            float buttonY = (TOOLBAR_HEIGHT - buttonHeight) * 0.5f;

            ImGui::SetCursorPos(ImVec2(viewport->Size.x - buttonWidth - rightPadding, buttonY));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.26f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

            if (ImGui::Button(speedText, ImVec2(buttonWidth, buttonHeight)))
            {
                sShowSpeedPopup = true;
                sSpeedPopupJustOpened = true;
                ImGui::OpenPopup("##SpeedPopup");
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            if (sShowSpeedPopup)
            {
                float totalBarHeight = MENUBAR_HEIGHT + TOOLBAR_HEIGHT;
                ImVec2 popupPos = ImVec2(viewport->Pos.x + viewport->Size.x - 220.0f - rightPadding,
                                         viewport->Pos.y + totalBarHeight + 2.0f);
                ImGui::SetNextWindowPos(popupPos);
                ImGui::SetNextWindowSize(ImVec2(220.0f, 0.0f));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
                ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.15f, 1.0f));

                ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                               ImGuiWindowFlags_AlwaysAutoResize;

                if (ImGui::BeginPopup("##SpeedPopup", popupFlags))
                {
                    ImGui::Text("Camera Speed");
                    ImGui::Spacing();

                    ImGui::SetNextItemWidth(196.0f);
                    ImGui::SliderFloat("##SpeedSlider", &state.camera.MovementSpeed, 1.0f, 500.0f, "%.1f");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Hold RMB + Scroll to adjust");

                    if (!sSpeedPopupJustOpened)
                    {
                        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_ChildWindows))
                        {
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            {
                                sShowSpeedPopup = false;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                    sSpeedPopupJustOpened = false;

                    ImGui::EndPopup();
                }
                else
                {
                    sShowSpeedPopup = false;
                }

                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);
            }
        }
        ImGui::End();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
}