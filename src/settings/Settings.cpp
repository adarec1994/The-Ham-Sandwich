#include "Settings.h"
#include "Keybinds.h"
#include "imgui.h"
#include <algorithm>
#include <unordered_map>
#include <fstream>

static bool sSettingsOpen = false;
static bool sCapturingKey = false;
static Keybinds::Action sCapturingAction = Keybinds::Action::COUNT;
static std::unordered_map<Keybinds::Action, Keybinds::Keybind> sTempBindings;
static bool sCaptureCtrl = false;
static bool sCaptureShift = false;
static bool sCaptureAlt = false;

static GraphicsSettings sGraphicsSettings;
static GraphicsSettings sTempGraphicsSettings;
static bool sGraphicsSettingsChanged = false;

GraphicsSettings& GetGraphicsSettings()
{
    return sGraphicsSettings;
}

void SaveGraphicsSettings()
{
    std::ofstream file("graphics_settings.cfg");
    if (file.is_open())
    {
        file << "anisotropic_filtering=" << (sGraphicsSettings.anisotropicFiltering ? 1 : 0) << "\n";
        file << "anisotropic_level=" << sGraphicsSettings.anisotropicLevel << "\n";
        file << "normal_maps=" << (sGraphicsSettings.normalMapsEnabled ? 1 : 0) << "\n";
    }
}

void LoadGraphicsSettings()
{
    std::ifstream file("graphics_settings.cfg");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            size_t eq = line.find('=');
            if (eq != std::string::npos)
            {
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);

                if (key == "anisotropic_filtering")
                    sGraphicsSettings.anisotropicFiltering = (std::stoi(value) != 0);
                else if (key == "anisotropic_level")
                    sGraphicsSettings.anisotropicLevel = std::clamp(std::stoi(value), 1, 16);
                else if (key == "normal_maps")
                    sGraphicsSettings.normalMapsEnabled = (std::stoi(value) != 0);
            }
        }
    }
}

static void InitTempBindings()
{
    sTempBindings = Keybinds::GetBindingsCopy();
    sTempGraphicsSettings = sGraphicsSettings;
    sGraphicsSettingsChanged = false;
}

static ImGuiKey CaptureKeyPress()
{
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
    {
        if (key == ImGuiKey_MouseLeft || key == ImGuiKey_MouseRight ||
            key == ImGuiKey_MouseMiddle || key == ImGuiKey_MouseX1 || key == ImGuiKey_MouseX2)
            continue;

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
    float maxHeight = 600.0f;
    float windowWidth = std::min(work_size.x * 0.8f, maxWidth);
    float windowHeight = std::min(work_size.y * 0.85f, maxHeight);

    if (work_size.x < 400.0f) windowWidth = work_size.x - 20.0f;
    if (work_size.y < 300.0f) windowHeight = work_size.y - 20.0f;

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    {
        if (ImGui::BeginTabBar("SettingsTabs"))
        {
            if (ImGui::BeginTabItem("Graphics"))
            {
                ImGui::Spacing();
                ImGui::Text("Texture Filtering");
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Checkbox("Anisotropic Filtering", &sTempGraphicsSettings.anisotropicFiltering))
                    sGraphicsSettingsChanged = true;

                if (sTempGraphicsSettings.anisotropicFiltering)
                {
                    ImGui::Indent();
                    ImGui::Text("Anisotropic Level:");
                    ImGui::SameLine();

                    const char* levels[] = { "1x", "2x", "4x", "8x", "16x" };
                    int levelIndex = 0;
                    switch (sTempGraphicsSettings.anisotropicLevel)
                    {
                        case 1: levelIndex = 0; break;
                        case 2: levelIndex = 1; break;
                        case 4: levelIndex = 2; break;
                        case 8: levelIndex = 3; break;
                        case 16: levelIndex = 4; break;
                        default: levelIndex = 4; break;
                    }

                    ImGui::SetNextItemWidth(80.0f);
                    if (ImGui::Combo("##AnisoLevel", &levelIndex, levels, 5))
                    {
                        const int levelValues[] = { 1, 2, 4, 8, 16 };
                        sTempGraphicsSettings.anisotropicLevel = levelValues[levelIndex];
                        sGraphicsSettingsChanged = true;
                    }
                    ImGui::Unindent();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Text("Normal Mapping");
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Checkbox("Enable Normal Maps", &sTempGraphicsSettings.normalMapsEnabled))
                    sGraphicsSettingsChanged = true;

                if (!sTempGraphicsSettings.normalMapsEnabled)
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(?)");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Disabling normal maps may improve performance\non older hardware.");
                }

                ImGui::Spacing();
                if (sGraphicsSettingsChanged)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "* Changes require restart to take effect");
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Keybindings"))
            {
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

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();

        float totalButtonWidth = 160.0f;
        float startX = (windowWidth - totalButtonWidth) * 0.5f - 8.0f;
        ImGui::SetCursorPosX(startX);

        if (ImGui::Button("Save", ImVec2(70, 0)))
        {
            Keybinds::ApplyBindings(sTempBindings);
            Keybinds::Save();

            sGraphicsSettings = sTempGraphicsSettings;
            SaveGraphicsSettings();

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