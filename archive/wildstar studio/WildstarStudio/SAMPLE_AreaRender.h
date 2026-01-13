#pragma once

#include "Matrix.h"
#include "AreaFile.h"
#include "PerspectivicCamera.h"

class AreaRender
{
	Matrix mView, mProj;

	uint32 mWidth;
	uint32 mHeight;

	GLuint mFrameBuffer;
	GLuint mTexture;
	GLuint mDepthBuffer;

	PerspectivicCameraPtr mCamera;
	Vector3 mPosition, mForward, mRight, mUp;

	POINT mLastCursor;
	uint32 mLastUpdate;

	void update();

public:
	AreaRender();

	void initGraphics(uint32 width, uint32 height);
	void resize(uint32 width, uint32 height);
	void renderArea(AreaFilePtr file);
	void onModelSelected(AreaFilePtr file);

	void yaw(float diff);
	void pitch(float diff);
	void roll(float diff);
	void moveForward(float amount);
	void moveSide(float amount);
	void moveUp(float amount);
	void move(const Vector3& direction);

	GLuint getTexture() const { return mTexture; }
};