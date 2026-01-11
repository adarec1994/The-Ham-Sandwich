#include "settings.h"
#include "imgui.h"

void RenderSettingsWindow(bool* p_open) {
    if (!*p_open) return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_size = ImVec2(work_size.x * 0.8f, work_size.y * 0.8f);

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    if (ImGui::Begin("Settings", p_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Application Settings");
        ImGui::Separator();
    }
    ImGui::End();
}