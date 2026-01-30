#pragma once
#include "UI.h"
#include <string>

namespace UI_Details
{
    void Draw(AppState& state);
    void Reset();

    void StartExportGLB(AppState& state, const std::string& defaultName);
    void StartExportFBX(AppState& state, const std::string& defaultName);
    bool IsExportInProgress();
    bool IsPropsLoadingInProgress();
}