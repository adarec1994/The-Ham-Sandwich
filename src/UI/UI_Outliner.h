#pragma once
#include "UI.h"

namespace UI_Outliner
{
    void Draw(AppState& state);
    void Reset();

    float GetWindowHeight();
    void SetWindowHeight(float height);
    float GetSidebarWidth();
    void SetSidebarWidth(float width);
}