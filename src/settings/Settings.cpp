#include "Settings.h"
#include "Keybinds.h"
#include "imgui.h"
#include <algorithm>
#include <unordered_map>

static bool sSettingsOpen = false;
static bool sCapturingKey = false;
static Keybinds::Action sCapturingAction = Keybinds::Action::COUNT;
static std::unordered_map<Keybinds::Action, Keybinds::Keybind> sTempBindings;
static bool sCaptureCtrl = false;
static bool sCaptureShift = false;
static bool sCaptureAlt = false;

static void InitTempBindings()
{
    sTempBindings = Keybinds::GetBindingsCopy();
}

static ImGuiKey CaptureKeyPress()
{
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
    {
        if (key == ImGuiKey_MouseLeft || key == ImGuiKey_MouseRight ||
            key == ImGuiKey_MouseMiddle || key == ImGuiKey_MouseX1 || key == ImGuiKey_MouseX2)
            continue;

        // Skip modifier keys themselves - we track them separately
        if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
            key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
            key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt)
            continue;

        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false))
        {
            return static_cast<ImGuiKey>(key);
        }
    }
    return ImGuiKey_None;
}

static std::string GetCaptureString()
{
    std::string result;
    if (sCaptureCtrl) result += "Ctrl+";
    if (sCaptureShift) result += "Shift+";
    if (sCaptureAlt) result += "Alt+";
    result += "...";
    return result;
}

void RenderSettingsWindow(bool* p_open) {
    if (!*p_open) return;

    if (!sSettingsOpen)
    {
        sSettingsOpen = true;
        sCapturingKey = false;
        sCapturingAction = Keybinds::Action::COUNT;
        InitTempBindings();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_size = viewport->WorkSize;

    float maxWidth = 500.0f;
    float maxHeight = 500.0f;
    float windowWidth = std::min(work_size.x * 0.8f, maxWidth);
    float windowHeight = std::min(work_size.y * 0.8f, maxHeight);

    if (work_size.x < 400.0f) windowWidth = work_size.x - 20.0f;
    if (work_size.y < 300.0f) windowHeight = work_size.y - 20.0f;

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("Keybindings");
        ImGui::Separator();
        ImGui::Spacing();

        float availWidth = ImGui::GetContentRegionAvail().x;
        float labelWidth = availWidth * 0.45f;
        float buttonWidth = 120.0f;

        if (ImGui::BeginChild("KeybindsList", ImVec2(0, -40), false))
        {
            for (int i = 0; i < static_cast<int>(Keybinds::Action::COUNT); i++)
            {
                Keybinds::Action action = static_cast<Keybinds::Action>(i);
                auto& bind = sTempBindings[action];

                ImGui::PushID(i);

                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", bind.displayName.c_str());

                ImGui::SameLine(labelWidth);

                bool isCapturing = sCapturingKey && sCapturingAction == action;

                if (isCapturing)
                {
                    // Track modifiers being held
                    sCaptureCtrl = ImGui::GetIO().KeyCtrl;
                    sCaptureShift = ImGui::GetIO().KeyShift;
                    sCaptureAlt = ImGui::GetIO().KeyAlt;

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.5f, 1.0f));

                    std::string captureStr = GetCaptureString();
                    ImGui::Button(captureStr.c_str(), ImVec2(buttonWidth, 0));

                    ImGui::PopStyleColor(3);

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        sCapturingKey = false;
                        sCapturingAction = Keybinds::Action::COUNT;
                    }

                    ImGuiKey pressed = CaptureKeyPress();
                    if (pressed != ImGuiKey_None && pressed != ImGuiKey_Escape)
                    {
                        bind.key = pressed;
                        bind.ctrl = sCaptureCtrl;
                        bind.shift = sCaptureShift;
                        bind.alt = sCaptureAlt;
                        sCapturingKey = false;
                        sCapturingAction = Keybinds::Action::COUNT;
                    }
                    else if (pressed == ImGuiKey_Escape)
                    {
                        sCapturingKey = false;
                        sCapturingAction = Keybinds::Action::COUNT;
                    }
                }
                else
                {
                    std::string keyStr;
                    if (bind.ctrl) keyStr += "Ctrl+";
                    if (bind.shift) keyStr += "Shift+";
                    if (bind.alt) keyStr += "Alt+";
                    keyStr += Keybinds::GetKeyName(bind.key);

                    if (ImGui::Button(keyStr.c_str(), ImVec2(buttonWidth, 0)))
                    {
                        sCapturingKey = true;
                        sCapturingAction = action;
                        sCaptureCtrl = false;
                        sCaptureShift = false;
                        sCaptureAlt = false;
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
        }

        ImGui::Separator();

        float totalButtonWidth = 160.0f;
        float startX = (windowWidth - totalButtonWidth) * 0.5f - 8.0f;
        ImGui::SetCursorPosX(startX);

        if (ImGui::Button("Save", ImVec2(70, 0)))
        {
            Keybinds::ApplyBindings(sTempBindings);
            Keybinds::Save();
            sSettingsOpen = false;
            sCapturingKey = false;
            *p_open = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(70, 0)))
        {
            sSettingsOpen = false;
            sCapturingKey = false;
            *p_open = false;
        }
    }
    ImGui::End();

    if (!*p_open)
    {
        sSettingsOpen = false;
        sCapturingKey = false;
    }
}