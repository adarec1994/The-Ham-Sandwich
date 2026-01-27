#pragma once
#include "UI.h"
#include <filesystem>

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_texture, int* out_width, int* out_height);
bool PathHasArchives(const std::string& path);
bool TryAutoLoadArchives(AppState& state);
void SaveLastUsedPath(const std::string& path);
std::string LoadLastUsedPath();
void LoadArchivesFromPath(AppState& state, const std::string& pathStr);
void RenderSplashScreen(AppState& state);

struct ArchiveLoadState {
    bool isLoading = false;
    bool scanComplete = false;
    std::vector<std::filesystem::path> pendingArchives;
    int totalArchives = 0;
    int loadedArchives = 0;
    std::string currentArchiveName;
    std::string loadPath;
    bool savePathOnComplete = false;
};

extern ArchiveLoadState gArchiveLoadState;