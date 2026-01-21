#include "splashscreen.h"
#include "../Archive.h"
#include "ImGuiFileDialog.h"
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static bool gSkipIconLoaded = false;
static GLuint gSkipIconTexture = 0;
static int gSkipIconWidth = 0;
static int gSkipIconHeight = 0;

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
            }
            catch (const std::exception& e) {
                std::cerr << "[Warning] Failed to load " << entry.path().string()
                          << ": " << e.what() << std::endl;
            }
        }
    }

    if (!state.archives.empty()) {
        state.archivesLoaded = true;
    }
}

void RenderSplashScreen(AppState& state) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("Splash", nullptr, flags)) {

        if (!gSkipIconLoaded) {
            gSkipIconLoaded = LoadTextureFromFile(
                "./assets/icons/RightArrow.png",
                &gSkipIconTexture, &gSkipIconWidth, &gSkipIconHeight);
        }

        if (gSkipIconLoaded) {
            float icon_size = 44.0f;
            float pad = 14.0f;

            ImGui::SetCursorPos(ImVec2(viewport->Size.x - icon_size - pad, pad));

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.22f, 0.26f, 0.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.35f, 0.45f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f, 0.20f, 0.24f, 0.70f));
            ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0, 0, 0, 0));

            if (ImGui::ImageButton("##SkipSplash",
                reinterpret_cast<void*>(static_cast<intptr_t>(gSkipIconTexture)),
                ImVec2(icon_size, icon_size),
                ImVec2(0, 0), ImVec2(1, 1),
                ImVec4(0, 0, 0, 0)))
            {
                state.archivesLoaded = true;
            }

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);
        }

        float centerX = viewport->Size.x * 0.5f;
        float centerY = viewport->Size.y * 0.5f;
        ImVec2 btnSize(250, 40);

        ImGui::SetCursorPos(ImVec2(centerX - btnSize.x * 0.5f,
                                   centerY - btnSize.y * 0.5f));

        if (ImGui::Button("Open Wildstar Folder", btnSize)) {
            IGFD::FileDialogConfig config;
            config.path = state.currentDialogPath;
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseDirDlgKey",
                "Choose Project Directory",
                nullptr,
                config);
        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_Always);

    if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey")) {

        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath =
                ImGuiFileDialog::Instance()->GetCurrentPath();

            state.currentDialogPath = filePath;
            LoadArchivesFromPath(state, filePath);
        }

        ImGuiFileDialog::Instance()->Close();
    }
}