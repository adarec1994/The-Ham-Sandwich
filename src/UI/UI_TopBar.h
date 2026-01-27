#pragma once
#include "UI.h"

namespace UI_TopBar
{
    void Draw(AppState& state, bool* showSettings, bool* showAbout, bool* requestQuit);
    float GetHeight();
}