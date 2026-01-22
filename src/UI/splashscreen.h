#pragma once
#include "UI.h"

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_texture, int* out_width, int* out_height);
void RenderSplashScreen(AppState& state);