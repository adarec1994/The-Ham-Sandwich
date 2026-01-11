#include "stdafx.h"
#include "Camera.h"

Camera::Camera() {
	mPosition = Vector3(0, 0, 0);
	mForward = Vector3(1, 0, 0);
	mUp = Vector3(0, 0, -1);
	mRight = Vector3(0, -1, 0);

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void Camera::setPosition(const Vector3& pos) {
	mPosition = pos;

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void Camera::setTarget(const Vector3& tar) {
	auto dir = tar - mPosition;
	dir.normalize();
	mForward = dir;

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void Camera::moveUp(float amount) {
	move(Vector3(0, 0, 1) * amount);
}

void Camera::moveForward(float amount) {
	move(mForward * amount);
}

void Camera::moveSide(float amount) {
	move(mRight * amount);
}

void Camera::move(const Vector3& direction) {
	mPosition += direction;

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void Camera::pitch(float angle) {
	Matrix matRot = Matrix::rotation(angle, mRight);
	mForward = matRot * mForward;
	mForward.normalize();
	mUp = matRot * mUp;
	mUp.normalize();

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void Camera::yaw(float angle) {
	Matrix matRot = Matrix::rotation(angle, Vector3::UnitZ);
	mForward = matRot * mForward;
	mForward.normalize();
	mUp = matRot * mUp;
	mUp.normalize();
	mRight = matRot * mRight;
	mRight.normalize();

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

void Camera::roll(float angle) {
	Matrix matRot = Matrix::rotation(angle, mForward);
	mUp = matRot * mForward;
	mUp.normalize();
	mRight = matRot * mRight;
	mRight.normalize();

	mView = Matrix::lookAt(mPosition, mPosition + mForward, mUp);
}

Vector3 Camera::unproject(const Vector3& mousePos) {
	/*double viewDoubles[16] = { 0 };
	double projDoubles[16] = { 0 };

	for (uint32 i = 0; i < 16; ++i) {
	viewDoubles[i] = mView.transposed()._m[i];
	projDoubles[i] = mProjection.transposed()._m[i];
	}

	int32 viewport[4] = { 0, 0, sWindow->getClientWidth(), sWindow->getClientHeight() };

	double x, y, z;

	gluUnProject(mousePos.X, sWindow->getClientHeight() - mousePos.Y, mousePos.Z, viewDoubles, projDoubles, viewport, &x, &y, &z);

	return Vector3((float) x, (float) y, (float) z);*/
	//throw System::InvalidOperation("unproject is not supported yet");
	throw std::exception("unproject is not supported yet");
}