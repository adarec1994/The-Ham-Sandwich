#pragma once

#include "M3Model.h"
#include "I3Model.h"
#include "Matrix.h"
#include "Program.h"
#include "InputGeometry.h"
#include "TextureInput.h"

class ModelRender
{
	struct I3Uniforms
	{
		uint32 matView;
		uint32 matProj;
		uint32 matWorld;
	};

	struct M3Uniforms
	{
		uint32 matView;
		uint32 matProj;
		uint32 matWorld;
	};

	uint32 mWidth;
	uint32 mHeight;

	GLuint mFrameBuffer;
	GLuint mTexture;
	GLuint mDepthBuffer;

	float mRotation;
	bool mIsRotating;
	bool mPointMode;
	bool mUseNormal;
	uint32 mI3Sector;

	ProgramPtr mI3Program;
	ProgramPtr mM3Program;
	I3Uniforms mI3Uniform;
	M3Uniforms mM3Uniform;
	InputGeometryPtr mM3Geometry;
	TextureInputPtr mM3TexInput;
	InputGeometryPtr mI3Geometry;
	TextureInputPtr mI3TexInput;

	std::chrono::high_resolution_clock::time_point mLastUpdate;
public:
	ModelRender();

	void initGraphics(uint32 width, uint32 height);
	void renderModel(M3ModelPtr model);
	void renderModel(I3ModelPtr model);
	void resize(uint32 width, uint32 height);
	
	void setI3Sector(uint32 sector) { mI3Sector = sector; }

	void stopRotation();
	void restartRotation();
	void resetRotation();

	void toggleViewMode() { mPointMode = !mPointMode; }
	void toggleNormals() { mUseNormal = !mUseNormal; }

	GLuint getTexture() const { return mTexture; }
};