#include "stdafx.h"
#include "PerspectivicCamera.h"

PerspectivicCamera::PerspectivicCamera() {
	mAspect = 1.0f;
	mFov = 45.0f;
	mNear = 4.0f;
	mFar = 500.0f;

	updateMatrix();
}

void PerspectivicCamera::updateMatrix() {
	mProjection = Matrix::perspective(mAspect, mFov, mNear, mFar);
}

Matrix PerspectivicCamera::getProjectionPitched(float amount) {
	return Matrix::perspective(mAspect, mFov, mNear + amount, mFar);
}