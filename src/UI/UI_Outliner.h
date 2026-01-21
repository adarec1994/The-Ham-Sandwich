#pragma once
#include "UI.h"

namespace UI_Outliner
{
    void Draw(AppState& state);
    void Reset();
    
    int GetSelectedIndex();
    void SetSelectedIndex(int index);
    
    float GetWindowHeight();
    void SetWindowHeight(float height);
    float GetSidebarWidth();
    void SetSidebarWidth(float width);
}