#pragma once

#include "Vector3.h"

class Matrix
{
public:
	Matrix();

	void setIdentity();

	void transpose();
	Matrix transposed() const;
	bool invert();
	float determinant();

	Matrix operator * (const Matrix& rhs) const;
	Vector3 operator * (const Vector3& rhs) const;

	operator const float* () const {
		return _m;
	}

	operator float* () {
		return _m;
	}

#pragma warning (push)
	// non standard extension, unnamed struct/union
#pragma warning (disable: 4201)
	union
	{
		float m[4][4];
		float _m[16];
		struct
		{
			float m11, m12, m13, m14;
			float m21, m22, m23, m24;
			float m31, m32, m33, m34;
			float m41, m42, m43, m44;
		};
	};
#pragma warning (pop)

	static Matrix multiply(const Matrix& lhs, const Matrix& rhs);


#pragma region Generating Functions

	static Matrix rotationCenter(float angle, const Vector3& axis, const Vector3& center);
	static Matrix rotation(float angle, const Vector3& axis);
	static Matrix rotationAxis(const Vector3& angles);
	static Matrix translation(float x, float y, float z);
	static Matrix translation(const Vector3& v);
	static Matrix scale(float x, float y, float z);
	static Matrix scale(const Vector3& scales) { return scale(scales.X, scales.Y, scales.Z); }
	static Matrix ortho(float left, float top, float right, float bottom, float zNear, float zFar);
	static Matrix perspective(float fovy, float aspect, float zNear, float zFar);
	static Matrix lookAt(const Vector3& eye, const Vector3& at, const Vector3& up);

#pragma endregion
};