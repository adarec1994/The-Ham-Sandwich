#include "about.h"
#include "imgui.h"

static const char* s_aboutText = R"(
Wildstar Studio 2
Author: Matthew Wakeman
Contact: mattwake94@gmail.com
Version: 1.0.0
Built with: OpenGL, GLFW, Glad, ImGui

Acknowledgements:
----------------------
LaMuerteDeLaPassion - Working on M3 formats and figuring out ohw to export them with animations.
URL: https://github.com/LaMuerteDeLaPassion/wildstar-export-2
Muazin Mugadr - For the original Wildstar Studio.
URL: https://bitbucket.org/mugadr_m/wildstar-studio/src/master/
Marble Bag - For the Nexus Vault application
URL: https://github.com/MarbleBag/NexusVault-CLI
Wildstar Community - Endearing to preserve this game.

Licensing Information:
----------------------
This software uses the following open-source libraries:

1. Dear ImGui (MIT License)
2. GLFW (zlib/libpng License)
3. GLAD (MIT License)
4. STB Image (MIT License / Public Domain)

Controls:
---------
- Right-Click + WASD: Move Camera
- Right-Click + Mouse: Pan Camera
- Scroll Wheel: Adjust Camera Speed
)";

void RenderAboutWindow(bool* p_open) {
    if (!*p_open) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_size = ImVec2(work_size.x * 0.8f, work_size.y * 0.8f);

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    if (ImGui::Begin("About", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {

        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGui::TextUnformatted(s_aboutText);
            ImGui::EndChild();
        }
    }
    ImGui::End();
}