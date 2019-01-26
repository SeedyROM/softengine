#pragma once

#include <memory>

struct Color {
	int R = 255;
	int G = 255;
	int B = 255;
};

struct Colorable {
	Color color;
};

struct Coordinate {
	int x = 0;
	int y = 0;
};

struct RotationMatrix;

struct Vec3 {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	Vec3();
	Vec3(float x, float y, float z);
	float magnitude();
	Vec3 unit();
	void rotate(const RotationMatrix& rotationMatrix);
	Vec3 operator +(const Vec3& vector) const;
	Vec3 operator -(const Vec3& vector) const;
};

struct RotationMatrix {
	float m11, m12, m13, m21, m22, m23, m31, m32, m33;

	static RotationMatrix calculate(const Vec3& rotation);
	RotationMatrix operator *(const RotationMatrix& rm) const;
	Vec3 operator *(const Vec3& v) const;
};

struct Vertex2d : Colorable {
	Coordinate coordinate;
	int depth;
};

struct Vertex3d : Colorable {
	Vec3 vector;
};

struct Triangle {
	Vertex2d vertices[3];
	void createVertex(int index, const Coordinate& coordinate, int depth, const Color& color);
};

struct Polygon {
	Vertex3d* vertices[3];
	void bindVertex(int index, Vertex3d* vertex);
};

