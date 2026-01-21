#pragma once
#include "UI.h"

namespace UI_ContentBrowser
{
    void Draw(AppState& state);
    void Toggle();
    bool IsOpen();
    float GetHeight();
    float GetBarHeight();
    void Reset();
}