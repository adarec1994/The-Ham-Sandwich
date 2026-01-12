#pragma once

#include "InputGeometry.h"
#include "TextureInput.h"

SHARED_FWD(Pipeline);

class Pipeline
{
	static PipelinePtr gInstance;

	InputGeometryPtr mGeometry;
public:
	void applyGeometry(InputGeometryPtr geometry);
	void render(TextureInputPtr textureInput = nullptr);
	void removeGeometry(InputGeometryPtr geometry);

	static PipelinePtr getInstance() {
		if (gInstance == nullptr) {
			gInstance = std::make_shared<Pipeline>();
		}

		return gInstance;
	}
};

#define sPipeline (Pipeline::getInstance())