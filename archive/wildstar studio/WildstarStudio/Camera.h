#pragma once

#include "Matrix.h"

class Camera
{
protected:
	Matrix mProjection;
	Matrix mView;
	Vector3 mPosition;
	Vector3 mForward;
	Vector3 mUp;
	Vector3 mRight;

public:
	Camera();

	virtual void roll(float angle);
	virtual void yaw(float angle);
	virtual void pitch(float angle);
	virtual void moveForward(float amount);
	virtual void moveSide(float amount);
	virtual void moveUp(float amount);
	virtual void move(const Vector3& direction);
	virtual void setPosition(const Vector3& pos);
	virtual void setTarget(const Vector3& target);
	virtual void setFarClip(float clip) = 0;
	virtual void setNearClip(float clip) = 0;
	virtual void setClip(float nearClip, float farClip) = 0;
	virtual void setAspect(float aspect) = 0;
	virtual void setFieldOfView(float fov) = 0;
	virtual Matrix getProjectionPitched(float amount) = 0;

	virtual Vector3 unproject(const Vector3& mousePos);

	virtual Vector3 getPosition() const { return mPosition; }

	const Matrix& getView() const { return mView; }
	const Matrix& getProjection() const { return mProjection; }
};