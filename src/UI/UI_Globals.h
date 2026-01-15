// src/UI/UI_Globals.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../Area/AreaFile.h"
#include "../models/M3Render.h"

extern std::vector<AreaFilePtr> gLoadedAreas;

extern AreaChunkRenderPtr gSelectedChunk;
extern int gSelectedChunkIndex;
extern int gSelectedAreaIndex;
extern std::string gSelectedChunkAreaName;

extern std::shared_ptr<M3Render> gLoadedModel;
