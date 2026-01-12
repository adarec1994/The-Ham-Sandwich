#include "SplashScreen.h"
#include "../Archive.h"
#include "ImGuiFileDialog.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

void LoadArchivesFromPath(AppState& state, const std::string& pathStr) {
    if (!fs::exists(pathStr) || !fs::is_directory(pathStr)) return;

    for (const auto& entry : fs::recursive_directory_iterator(pathStr)) {
        if (entry.is_regular_file() && entry.path().extension() == ".index") {
            std::string filename = entry.path().filename().string();

            if (filename == "Bootstrap.index" ||
                filename == "Launcher.index" ||
                filename == "Client64.index" ||
                filename == "Patch.index") {
                continue;
            }

            try {
                std::cout << "Loading: " << entry.path() << std::endl;

                std::wstring wpath = entry.path().wstring();

                auto archive = std::make_shared<Archive>(wpath);
                archive->loadIndexInfo();
                archive->loadArchiveInfo();
                archive->asyncLoad();

                state.archives.push_back(archive);
            } catch (const std::exception& e) {
                std::cerr << "[Warning] Failed to load " << entry.path().string() << ": " << e.what() << std::endl;
            }
        }
    }

    if (!state.archives.empty()) {
        state.archivesLoaded = true;
        state.sidebar_visible = true;
    }
}

void RenderSplashScreen(AppState& state) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("Splash", nullptr, flags)) {

        float centerX = viewport->Size.x * 0.5f;
        float centerY = viewport->Size.y * 0.5f;
        ImVec2 btnSize(250, 40);

        ImGui::SetCursorPos(ImVec2(centerX - btnSize.x * 0.5f, centerY - btnSize.y * 0.5f));

        if (ImGui::Button("Open Project Folder", btnSize)) {
            IGFD::FileDialogConfig config;
            config.path = state.currentDialogPath;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose Project Directory", nullptr, config);
        }
    }
    ImGui::End();

    if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey")) {

        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            state.currentDialogPath = filePath;
            LoadArchivesFromPath(state, filePath);
        }

        ImGuiFileDialog::Instance()->Close();
    }
}