#pragma once

#include "Camera.h"

class PerspectivicCamera : public Camera
{
	float mAspect;
	float mFov;
	float mNear, mFar;

	void updateMatrix();
public:
	PerspectivicCamera();

	virtual void setFarClip(float clip) {
		mFar = clip;
		updateMatrix();
	}

	virtual void setNearClip(float clip) {
		mNear = clip;
		updateMatrix();
	}

	virtual void setClip(float nearClip, float farClip) {
		mNear = nearClip;
		mFar = farClip;
		updateMatrix();
	}

	virtual void setAspect(float aspect) {
		mAspect = aspect;
		updateMatrix();
	}

	virtual void setFieldOfView(float fov) {
		mFov = fov;
		updateMatrix();
	}

	virtual Matrix getProjectionPitched(float amount);
};

SHARED_TYPE(PerspectivicCamera);