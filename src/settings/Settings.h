#pragma once

struct GraphicsSettings
{
    bool anisotropicFiltering = true;
    int anisotropicLevel = 16;
    bool normalMapsEnabled = true;
};

GraphicsSettings& GetGraphicsSettings();
void SaveGraphicsSettings();
void LoadGraphicsSettings();

void RenderSettingsWindow(bool* p_open);