#include "StdAfx.h"
#include "Matrix.h"

Matrix::Matrix() {
	setIdentity();
}

void Matrix::setIdentity() {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (i == j)
				m[i][j] = 1;
			else
				m[i][j] = 0;
		}
	}
}

Matrix Matrix::operator * (const Matrix& rhs) const {
	Matrix ret;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			ret.m[i][j] =
				m[i][0] * rhs.m[0][j] +
				m[i][1] * rhs.m[1][j] +
				m[i][2] * rhs.m[2][j] +
				m[i][3] * rhs.m[3][j];
		}
	}
	return ret;
}

Vector3 Matrix::operator * (const Vector3& rhs) const {
	Vector3 ret;
	ret.X = rhs.X * m11 + rhs.Y * m12 + rhs.Z * m13 + m14;
	ret.Y = rhs.X * m21 + rhs.Y * m22 + rhs.Z * m23 + m24;
	ret.Z = rhs.X * m31 + rhs.Y * m32 + rhs.Z * m33 + m34;

	float w = 1.0f / (rhs.X * m41 + rhs.Y * m42 + rhs.Z * m43 + m44);

	ret *= w;

	return ret;
}

Matrix Matrix::transposed() const {
	Matrix ret = *this;
	ret.transpose();
	return ret;
}

void Matrix::transpose() {
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < i; ++j) {
			float tmp = m[j][i];
			m[j][i] = m[i][j];
			m[i][j] = tmp;
		}
	}
}

float Matrix::determinant() {
	return m11 * m22 * m33 * m44 + m11 * m23 * m34 * m42 + m11 * m24 * m32 * m43
		+ m12 * m21 * m34 * m43 + m12 * m23 * m31 * m44 + m12 * m24 * m33 * m41
		+ m13 * m21 * m32 * m44 + m13 * m22 * m34 * m41 + m13 * m24 * m31 * m42
		+ m14 * m21 * m33 * m42 + m14 * m22 * m31 * m43 + m14 * m23 * m32 * m41
		- m11 * m22 * m34 * m43 - m11 * m23 * m32 * m44 - m11 * m24 * m33 * m42
		- m12 * m21 * m33 * m44 - m12 * m23 * m34 * m41 - m12 * m24 * m31 * m43
		- m13 * m21 * m34 * m42 - m13 * m22 * m31 * m44 - m13 * m24 * m32 * m41
		- m14 * m21 * m32 * m43 - m14 * m22 * m33 * m41 - m14 * m23 * m31 * m42;
}

bool Matrix::invert() {
	float det = determinant();
	if (det == 0.0f)
		return false;

	float invDet = 1.0f / det;

	__declspec(thread) static float b[4][4];

	b[0][0] = m22 * m33 * m44 + m23 * m34 * m42 + m24 * m32 * m43 - m22 * m34 * m43 - m23 * m32 * m44 - m24 * m33 * m42;
	b[0][1] = m12 * m34 * m43 + m13 * m32 * m44 + m14 * m33 * m42 - m12 * m33 * m44 - m13 * m34 * m42 - m14 * m32 * m43;
	b[0][2] = m12 * m23 * m44 + m13 * m24 * m42 + m14 * m22 * m43 - m12 * m24 * m43 - m13 * m22 * m44 - m14 * m23 * m42;
	b[0][3] = m12 * m24 * m33 + m13 * m22 * m34 + m14 * m23 * m32 - m12 * m23 * m34 - m13 * m24 * m32 - m14 * m22 * m33;

	b[1][0] = m21 * m34 * m43 + m23 * m31 * m44 + m24 * m33 * m41 - m21 * m33 * m44 - m23 * m34 * m41 - m24 * m31 * m43;
	b[1][1] = m11 * m33 * m44 + m13 * m34 * m41 + m14 * m31 * m43 - m11 * m34 * m43 - m13 * m31 * m44 - m14 * m33 * m41;
	b[1][2] = m11 * m24 * m43 + m13 * m21 * m44 + m14 * m23 * m41 - m11 * m23 * m44 - m13 * m24 * m41 - m14 * m21 * m43;
	b[1][3] = m11 * m23 * m34 + m13 * m24 * m31 + m14 * m21 * m33 - m11 * m24 * m33 - m13 * m21 * m34 - m14 * m23 * m31;

	b[2][0] = m21 * m32 * m44 + m22 * m34 * m41 + m24 * m31 * m42 - m21 * m34 * m42 - m22 * m31 * m44 - m24 * m32 * m41;
	b[2][1] = m11 * m34 * m42 + m12 * m31 * m44 + m14 * m32 * m41 - m11 * m32 * m44 - m12 * m34 * m41 - m14 * m32 * m42;
	b[2][2] = m11 * m22 * m44 + m12 * m24 * m41 + m14 * m21 * m42 - m11 * m24 * m42 - m12 * m21 * m44 - m14 * m22 * m41;
	b[2][3] = m11 * m24 * m32 + m12 * m21 * m34 + m14 * m22 * m31 - m11 * m22 * m34 - m12 * m24 * m31 - m14 * m21 * m32;

	b[3][0] = m21 * m33 * m42 + m22 * m31 * m43 + m23 * m32 * m41 - m21 * m32 * m43 - m22 * m33 * m41 - m23 * m31 * m42;
	b[3][1] = m11 * m32 * m43 + m12 * m33 * m41 + m13 * m31 * m42 - m11 * m33 * m42 - m12 * m31 * m43 - m13 * m32 * m41;
	b[3][2] = m11 * m23 * m42 + m12 * m21 * m43 + m13 * m22 * m41 - m11 * m22 * m43 - m12 * m23 * m41 - m13 * m21 * m42;
	b[3][3] = m11 * m22 * m33 + m12 * m23 * m31 + m13 * m21 * m32 - m11 * m23 * m32 - m12 * m21 * m33 - m13 * m22 * m31;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j)
			m[i][j] = b[i][j] * invDet;
	}

	return true;
}

Matrix Matrix::multiply(const Matrix& lhs, const Matrix& rhs) {
	Matrix ret;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			ret.m[i][j] =
				lhs.m[i][0] * rhs.m[0][j] +
				lhs.m[i][1] * rhs.m[1][j] +
				lhs.m[i][2] * rhs.m[2][j] +
				lhs.m[i][3] * rhs.m[3][j];
		}
	}

	return ret;
}

Matrix Matrix::scale(float x, float y, float z) {
	Matrix ret;
	ret.m11 = x;
	ret.m22 = y;
	ret.m33 = z;

	return ret;
}

Matrix Matrix::translation(float x, float y, float z) {
	Matrix ret;
	ret.m14 = x;
	ret.m24 = y;
	ret.m34 = z;

	return ret;
}

Matrix Matrix::translation(const Vector3& v) {
	return translation(v.X, v.Y, v.Z);
}

Matrix Matrix::rotation(float angle, const Vector3& axis) {
	angle = (angle * 3.141592f) / 180.0f;
	float cosv = cosf(angle);
	float sinv = sinf(angle);
	float ux = axis.X;
	float uy = axis.Y;
	float uz = axis.Z;
	float ux2 = ux * ux;
	float uy2 = uy * uy;
	float uz2 = uz * uz;

	Matrix ret;
	ret.m11 = cosv + ux2 * (1.0f - cosv);
	ret.m12 = ux * uy * (1 - cosv) - uz * sinv;
	ret.m13 = ux * uz * (1 - cosv) + uy * sinv;
	ret.m21 = uy * ux * (1 - cosv) + uz * sinv;
	ret.m22 = cosv + uy2 * (1 - cosv);
	ret.m23 = uy * uz * (1 - cosv) - ux * sinv;
	ret.m31 = uz * ux * (1 - cosv) - uy * sinv;
	ret.m32 = uz * uy * (1 - cosv) + ux * sinv;
	ret.m33 = cosv + uz2 * (1 - cosv);

	return ret;
}

Matrix Matrix::rotationAxis(const Vector3& v) {
	return rotation(v.X, Vector3::UnitX) * rotation(v.Y, Vector3::UnitY) * rotation(v.Z, Vector3::UnitZ);
}

Matrix Matrix::rotationCenter(float angle, const Vector3& axis, const Vector3& center) {
	Matrix toCenter = Matrix::translation(Vector3::negate(center));
	Matrix fromCenter = Matrix::translation(center);
	Matrix rotation = Matrix::rotation(angle, axis);

	Matrix ret;
	ret = Matrix::multiply(ret, fromCenter);
	ret = Matrix::multiply(ret, rotation);
	ret = Matrix::multiply(ret, toCenter);

	return ret;
}

Matrix Matrix::ortho(float left, float top, float right, float bottom, float zNear, float zFar) {
	Matrix ret;
	ret.m11 = 2.0f / (right - left);
	ret.m22 = 2.0f / (top - bottom);
	ret.m33 = -2.0f / (zFar - zNear);
	ret.m14 = -1.0f * ((right + left) / (right - left));
	ret.m24 = -1.0f * ((top + bottom) / (top - bottom));
	ret.m34 = -1.0f * ((zFar + zNear) / (zFar - zNear));

	return ret;
}

Matrix Matrix::perspective(float fovy, float aspect, float zNear, float zFar) {
	Matrix ret;
	float top = zNear * tanf(fovy * 3.141592f / 360.0f);
	float bottom = -top;
	float left = bottom * aspect;
	float right = top * aspect;
	ret.m11 = 2.0f * zNear / (right - left);
	ret.m13 = (right + left) / (right - left);
	ret.m22 = 2.0f * zNear / (top - bottom);
	ret.m23 = (top + bottom) / (top - bottom);
	ret.m33 = -(zFar + zNear) / (zFar - zNear);
	ret.m34 = -2.0f * zFar * zNear / (zFar - zNear);
	ret.m43 = -1;
	ret.m44 = 0.0f;

	return ret;
}

Matrix Matrix::lookAt(const Vector3& eye, const Vector3& at, const Vector3& up) {
	Vector3 forward = eye - at;
	forward.normalize();
	Vector3 side = Vector3::negate(forward.cross(up));
	side.normalize();

	Matrix ret;

	float floats[] = {
		side.X, up.X, forward.X, 0,
		side.Y, up.Y, forward.Y, 0,
		side.Z, up.Z, forward.Z, 0,
		-side.dot(eye), -up.dot(eye), -forward.dot(eye), 1

	};

	memcpy(ret._m, floats, sizeof(floats));

	ret.transpose();
	return ret;
}