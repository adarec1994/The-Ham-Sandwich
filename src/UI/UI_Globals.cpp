// src/UI/UI_Globals.cpp
#include "UI_Globals.h"

std::vector<AreaFilePtr> gLoadedAreas;

AreaChunkRenderPtr gSelectedChunk = nullptr;
int gSelectedChunkIndex = -1;
int gSelectedAreaIndex = -1;
std::string gSelectedChunkAreaName;

std::shared_ptr<M3Render> gLoadedModel = nullptr;
