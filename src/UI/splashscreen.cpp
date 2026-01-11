#include "SplashScreen.h"
#include "../Archive.h" // CHANGED: Was ../Archive/Archive.h
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void RenderSplashScreen(AppState& state) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("Splash", nullptr, flags)) {

        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize("Project Brainwave").x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::SetCursorPosY(ImGui::GetWindowSize().y * 0.2f);
        ImGui::Text("Project Brainwave");

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 20));

        ImGui::Text("WildStar Installation Directory:");
        ImGui::InputText("##Path", state.searchPath, 512);

        ImGui::Dummy(ImVec2(0, 20));

        float btnWidth = 200.0f;
        ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);

        if (ImGui::Button("Load Archives", ImVec2(btnWidth, 40))) {
            std::string path(state.searchPath);
            if (fs::exists(path) && fs::is_directory(path)) {
                try {
                    for (const auto& entry : fs::recursive_directory_iterator(path)) {
                        if (entry.is_regular_file() && entry.path().extension() == ".index") {
                            std::cout << "Loading: " << entry.path() << std::endl;

                            std::wstring wpath = entry.path().wstring();

                            auto archive = std::make_shared<Archive>(wpath);
                            archive->loadIndexInfo();
                            archive->loadArchiveInfo();
                            archive->asyncLoad();

                            state.archives.push_back(archive);
                        }
                    }
                    if (!state.archives.empty()) {
                        state.archivesLoaded = true;
                        state.sidebar_visible = true;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error scanning directory: " << e.what() << std::endl;
                }
            }
        }
    }
    ImGui::End();
}