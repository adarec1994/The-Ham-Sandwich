#include "splashscreen.h"
#include "../Archive.h"
#include "ImGuiFileDialog.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static bool gSkipIconLoaded = false;
static ID3D11ShaderResourceView* gSkipIconTexture = nullptr;
static int gSkipIconWidth = 0;
static int gSkipIconHeight = 0;
static bool gAutoLoadAttempted = false;
static const char* CONFIG_FILENAME = "wildstar_path.cfg";

ArchiveLoadState gArchiveLoadState;

static const std::vector<std::string> DEFAULT_PATHS = {
    R"(C:\Program Files (x86)\NCSOFT\WildStar)",
    R"(C:\Program Files (x86)\Steam\steamapps\common\WildStar)",
    R"(C:\Program Files\NCSOFT\WildStar)",
    R"(C:\Program Files\Steam\steamapps\common\WildStar)"
};

std::string LoadLastUsedPath() {
    std::ifstream file(CONFIG_FILENAME);
    if (!file.is_open()) {
        return "";
    }

    std::string path;
    std::getline(file, path);
    file.close();

    while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' ')) {
        path.pop_back();
    }

    return path;
}

void SaveLastUsedPath(const std::string& path) {
    std::ofstream file(CONFIG_FILENAME);
    if (file.is_open()) {
        file << path;
        file.close();
    }
}

bool PathHasArchives(const std::string& pathStr) {
    if (!fs::exists(pathStr) || !fs::is_directory(pathStr)) {
        return false;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(pathStr)) {
            if (entry.is_regular_file() && entry.path().extension() == ".index") {
                std::string filename = entry.path().filename().string();

                if (filename == "Bootstrap.index" ||
                    filename == "Launcher.index" ||
                    filename == "Client64.index" ||
                    filename == "Patch.index") {
                    continue;
                }

                return true;
            }
        }
    } catch (const std::exception&) {
    }

    return false;
}

static void ScanForArchives(const std::string& pathStr) {
    gArchiveLoadState.pendingArchives.clear();
    gArchiveLoadState.totalArchives = 0;
    gArchiveLoadState.loadedArchives = 0;
    gArchiveLoadState.scanComplete = false;

    if (!fs::exists(pathStr) || !fs::is_directory(pathStr)) {
        gArchiveLoadState.scanComplete = true;
        return;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(pathStr)) {
            if (entry.is_regular_file() && entry.path().extension() == ".index") {
                std::string filename = entry.path().filename().string();

                if (filename == "Bootstrap.index" ||
                    filename == "Launcher.index" ||
                    filename == "Client64.index" ||
                    filename == "Patch.index") {
                    continue;
                }

                gArchiveLoadState.pendingArchives.push_back(entry.path());
            }
        }
    } catch (const std::exception&) {
    }

    gArchiveLoadState.totalArchives = static_cast<int>(gArchiveLoadState.pendingArchives.size());
    gArchiveLoadState.scanComplete = true;
}

static void StartArchiveLoading(AppState& state, const std::string& pathStr, bool savePath) {
    gArchiveLoadState.isLoading = true;
    gArchiveLoadState.loadPath = pathStr;
    gArchiveLoadState.savePathOnComplete = savePath;
    gArchiveLoadState.currentArchiveName = "Scanning...";
    gArchiveLoadState.phase = LoadPhase::ScanningArchives;
    gArchiveLoadState.audioStatusText.clear();
    state.currentDialogPath = pathStr;
    ScanForArchives(pathStr);
    gArchiveLoadState.phase = LoadPhase::LoadingArchives;
}

static void ProcessArchiveLoading(AppState& state) {
    if (!gArchiveLoadState.isLoading) return;

    if (gArchiveLoadState.phase == LoadPhase::LoadingArchives) {
        if (!gArchiveLoadState.scanComplete) return;

        if (gArchiveLoadState.loadedArchives >= gArchiveLoadState.totalArchives) {
            if (!state.archives.empty()) {
                gArchiveLoadState.phase = LoadPhase::InitializingAudio;
                gArchiveLoadState.audioStatusText = "Initializing audio database...";
                state.audioInitRequested = true;
            } else {
                gArchiveLoadState.isLoading = false;
                gArchiveLoadState.phase = LoadPhase::Idle;
            }
            return;
        }

        const auto& archivePath = gArchiveLoadState.pendingArchives[gArchiveLoadState.loadedArchives];
        gArchiveLoadState.currentArchiveName = archivePath.filename().string();

        try {
            std::wstring wpath = archivePath.wstring();

            auto archive = std::make_shared<Archive>(wpath);
            archive->loadIndexInfo();
            archive->loadArchiveInfo();
            archive->asyncLoad();

            state.archives.push_back(archive);
        }
        catch (const std::exception&) {
        }

        gArchiveLoadState.loadedArchives++;
    }
    else if (gArchiveLoadState.phase == LoadPhase::InitializingAudio) {
        if (state.audioInitComplete) {
            gArchiveLoadState.phase = LoadPhase::Complete;
            gArchiveLoadState.isLoading = false;
            state.archivesLoaded = true;

            if (gArchiveLoadState.savePathOnComplete) {
                SaveLastUsedPath(gArchiveLoadState.loadPath);
            }
        } else {
            if (!state.audioInitStatus.empty()) {
                gArchiveLoadState.audioStatusText = state.audioInitStatus;
            }
        }
    }
}

bool TryAutoLoadArchives(AppState& state) {
    for (const auto& path : DEFAULT_PATHS) {
        if (PathHasArchives(path)) {
            StartArchiveLoading(state, path, false);
            return true;
        }
    }

    std::string savedPath = LoadLastUsedPath();
    if (!savedPath.empty()) {
        if (PathHasArchives(savedPath)) {
            StartArchiveLoading(state, savedPath, false);
            return true;
        }
    }

    return false;
}

void LoadArchivesFromPath(AppState& state, const std::string& pathStr) {
    StartArchiveLoading(state, pathStr, true);
}

static void RenderLoadingBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowBgAlpha(0.8f);

    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##ArchiveLoadOverlay", nullptr, overlayFlags);
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    float windowWidth = 450.0f;
    ImVec2 windowPos(
        viewport->Pos.x + (viewport->Size.x - windowWidth) * 0.5f,
        viewport->Pos.y + (viewport->Size.y - 120.0f) * 0.5f
    );

    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, 0));

    ImGuiWindowFlags loadingFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("##ArchiveLoadWindow", nullptr, loadingFlags)) {
        if (gArchiveLoadState.phase == LoadPhase::LoadingArchives ||
            gArchiveLoadState.phase == LoadPhase::ScanningArchives) {
            ImGui::Text("Loading Archives...");
            ImGui::Spacing();

            float progress = gArchiveLoadState.totalArchives > 0
                ? static_cast<float>(gArchiveLoadState.loadedArchives) / static_cast<float>(gArchiveLoadState.totalArchives)
                : 0.0f;

            ImGui::ProgressBar(progress, ImVec2(windowWidth - 40.0f, 20));

            ImGui::Spacing();

            char countText[64];
            snprintf(countText, sizeof(countText), "%d / %d",
                     gArchiveLoadState.loadedArchives, gArchiveLoadState.totalArchives);
            ImGui::Text("%s", countText);

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s",
                              gArchiveLoadState.currentArchiveName.c_str());
        }
        else if (gArchiveLoadState.phase == LoadPhase::InitializingAudio) {
            ImGui::Text("Initializing Audio...");
            ImGui::Spacing();

            float time = static_cast<float>(ImGui::GetTime());
            float progress = (sinf(time * 2.0f) + 1.0f) * 0.5f;
            ImGui::ProgressBar(progress, ImVec2(windowWidth - 40.0f, 20), "");

            ImGui::Spacing();

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s",
                              gArchiveLoadState.audioStatusText.c_str());
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
}

void RenderSplashScreen(AppState& state) {
    if (!gAutoLoadAttempted) {
        gAutoLoadAttempted = true;
        TryAutoLoadArchives(state);
    }

    ProcessArchiveLoading(state);

    if (gArchiveLoadState.isLoading) {
        RenderLoadingBar();
        return;
    }

    if (state.archivesLoaded) {
        return;
    }

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

        if (gSkipIconLoaded && gSkipIconTexture) {
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
                reinterpret_cast<ImTextureID>(gSkipIconTexture),
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