#include "Settings.h"
#include "imgui.h"
#include <algorithm>

void RenderSettingsWindow(bool* p_open) {
    if (!*p_open) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_size = viewport->WorkSize;

    float maxWidth = 800.0f;
    float maxHeight = 600.0f;
    float windowWidth = std::min(work_size.x * 0.8f, maxWidth);
    float windowHeight = std::min(work_size.y * 0.8f, maxHeight);

    if (work_size.x < 400.0f) windowWidth = work_size.x - 20.0f;
    if (work_size.y < 300.0f) windowHeight = work_size.y - 20.0f;

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

    if (ImGui::Begin("Settings", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Application Settings");
        ImGui::Separator();
    }
    ImGui::End();
}