#pragma once
#include "UI.h"
#include <string>

namespace UI_ContentBrowser
{
    void Draw(AppState& state);
    void Toggle();
    bool IsOpen();
    bool IsDocked();
    void HideIfNotDocked();
    float GetHeight();
    float GetBarHeight();
    void Reset();
    void NavigateToPath(AppState& state, const std::string& folderPath);
}