#pragma once

class Vector2
{
public:
	Vector2() : X(0), Y(0) {
	}

	Vector2(float x, float y) : X(x), Y(y) {
	}

	operator const float* () const {
		return &X;
	}

	operator float* () {
		return &X;
	}

	float length() const {
		return sqrtf(X * X + Y * Y);
	}

	float lengthSquared() const {
		return X * X + Y * Y;
	}

	Vector2 operator - (const Vector2& r) {
		return Vector2(X - r.X, Y - r.Y);
	}

	Vector2 operator + (const Vector2& r) {
		return Vector2(X + r.X, Y + r.Y);
	}

	Vector2& operator += (const Vector2& r) {
		X += r.X;
		Y += r.Y;

		return *this;
	}

	Vector2& operator -= (const Vector2& r) {
		X -= r.X;
		Y -= r.Y;

		return *this;
	}

	void takeBigger(const Vector2& r) {
		if (r.X > X)
			X = r.X;
		if (r.Y > Y)
			Y = r.Y;
	}

	float X, Y;

	static bool isBetween(const Vector2& v/*ector*/, const Vector2& p/*osition*/, const Vector2& s/*ize*/) {
		return (v.X >= p.X && v.X <= p.X + s.X && v.Y >= p.Y && v.Y <= p.Y + s.Y);
	}
};

class Vector3
{
public:
	Vector3() : X(0), Y(0), Z(0) {
	}

	Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {
	}

	void normalize() {
		float len = length();
		X /= len;
		Y /= len;
		Z /= len;
	}

	Vector3 normalized() const {
		float len = length();
		return Vector3(X / len, Y / len, Z / len);
	}

	// takes the lesser value of this and other for each coordinate as new coordinate
	void takeMin(const Vector3& other) {
		if (other.X < X)
			X = other.X;
		if (other.Y < Y)
			Y = other.Y;
		if (other.Z < Z)
			Z = other.Z;
	}

	// takes the greater value of this and other for each coordinate as new coordinate
	void takeMax(const Vector3& other) {
		if (other.X > X)
			X = other.X;
		if (other.Y > Y)
			Y = other.Y;
		if (other.Z > Z)
			Z = other.Z;
	}

	float lengthSquared() const {
		return X * X + Y * Y + Z * Z;
	}

	float length() const {
		return sqrt(lengthSquared());
	}

	float dot(const Vector3& lhs) const {
		return (X * lhs.X + Y * lhs.Y + Z * lhs.Z);
	}

	Vector3 cross(const Vector3& lhs) const {
		return Vector3(
			Y * lhs.Z - Z * lhs.Y,
			Z * lhs.X - X * lhs.Z,
			X * lhs.Y - Y * lhs.X
			);
	}

	static Vector3 cross(const Vector3& lhs, const Vector3& rhs) {
		return lhs.cross(rhs);
	}

	Vector3 operator + (const Vector3& lhs) const {
		return Vector3(X + lhs.X, Y + lhs.Y, Z + lhs.Z);
	}

	Vector3& operator += (const Vector3& lhs) {
		X += lhs.X;
		Y += lhs.Y;
		Z += lhs.Z;

		return *this;
	}

	Vector3 operator - (const Vector3& lhs) const {
		return Vector3(X - lhs.X, Y - lhs.Y, Z - lhs.Z);
	}

	Vector3& operator -= (const Vector3& lhs) {
		X -= lhs.X;
		Y -= lhs.Y;
		Z -= lhs.Z;

		return *this;
	}

	Vector3 operator / (float val) const {
		return Vector3(X / val, Y / val, Z / val);
	}

	Vector3& operator /= (float val) {
		X /= val;
		Y /= val;
		Z /= val;

		return *this;
	}

	Vector3 operator * (float val) const {
		return Vector3(X * val, Y * val, Z * val);
	}

	Vector3& operator *= (float val) {
		X *= val;
		Y *= val;
		Z *= val;

		return *this;
	}

	operator const float* () const {
		return &X;
	}

	operator float* () {
		return &X;
	}

	uint32 toRGBX() const {
		uint32 r = (uint32) (X * 255.0f);
		uint32 g = (uint32) (Y * 255.0f);
		uint32 b = (uint32) (Z * 255.0f);
		uint32 a = 0xFF;

		return r | (g << 8) | (b << 16) | (a << 24);
	}

	float X, Y, Z;

	static Vector3 UnitX, UnitY, UnitZ;

	static Vector3 negate(const Vector3& v) {
		return Vector3(-v.X, -v.Y, -v.Z);
	}
};

class Vector4
{
public:
	Vector4() : X(0), Y(0), Z(0), W(0) {
	}

	Vector4(float x, float y, float z, float w) {
		X = x;
		Y = y;
		Z = z;
		W = w;
	}

	float X, Y, Z, W;
};