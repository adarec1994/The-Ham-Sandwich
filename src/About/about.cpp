#include "about.h"
#include "imgui.h"
#include <algorithm>

static const char* s_aboutText = R"(
Wildstar Studio 2
Author: Matthew Wakeman
Contact: mattwake94@gmail.com
Version: 1.0.0
Built with: OpenGL, GLFW, Glad, ImGui

About:
----------------------
This software was built in order to provide a more up-to-date, clean way of browsing the contents of the WildStar archive files.
No reverse engineering work has been done by me, and those who have done this work are listed in the Acknowledgements section.
I have simply taken their work, converted/ modernized it in C++, and presented it as an all in one solution.

Most, if not all, aspects of this program are subject to change as it continues to be iterated on and developed.
Everything should be considered WIP.

Acknowledgements:
----------------------
LaMuerteDeLaPassion - M3 format and general support,
URL: https://github.com/LaMuerteDeLaPassion/wildstar-export-2
Muazin Mugadr - Original Wildstar Studio.
URL: https://bitbucket.org/mugadr_m/wildstar-studio/src/master/
Marble Bag - Nexus Vault application
URL: https://github.com/MarbleBag/NexusVault-CLI
Zee - Reverse engineered many of the formats.
URL: https://github.com/CucFlavius/ws-tool
Wildstar Community - Endearing to preserve this game.

Licensing Information:
----------------------
This software uses the following open-source libraries:

1. Dear ImGui
2. GLFW
3. GLAD
4. STB Image

)";

void RenderAboutWindow(bool* p_open) {
    if (!*p_open) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_size = viewport->WorkSize;

    float maxWidth = 700.0f;
    float maxHeight = 550.0f;
    float windowWidth = std::min(work_size.x * 0.8f, maxWidth);
    float windowHeight = std::min(work_size.y * 0.8f, maxHeight);

    if (work_size.x < 400.0f) windowWidth = work_size.x - 20.0f;
    if (work_size.y < 300.0f) windowHeight = work_size.y - 20.0f;

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

    if (ImGui::Begin("About", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {

        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGui::TextUnformatted(s_aboutText);
            ImGui::EndChild();
        }
    }
    ImGui::End();
}