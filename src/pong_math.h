#ifndef PONG_MATH_H
#define PONG_MATH_H

struct v2 {
	float x, y;
};

struct v2i {
	int x, y;
};

struct rectangle2i {
	int minX, minY;
	int maxX, maxY;
};

v2i roundV2(v2 input) {
	v2i result = {};

	result.x = (int)round(input.x);
	result.y = (int)round(input.y);

	return result;
}

inline v2 V2(float x, float y) {
	v2 result;

	result.x = x;
	result.y = y;

	return result;
}

inline v2 operator+(v2 a, v2 b) {
	v2 result;
	
	result.x = a.x + b.x;
	result.y = a.y + b.y;

	return result;
}

inline v2 & operator+=(v2 &a, v2 b) {
	a = a + b;

	return a;
}

inline v2 operator-(v2 a, v2 b) {
	v2 result;

	result.x = a.x - b.x;
	result.y = a.y - b.y;

	return result;
}

inline v2 operator-=(v2 &a, v2 b) {
	a = a - b;

	return a;
}

inline v2 operator*(float s, v2 a) {
	v2 result;

	result.x = s * a.x;
	result.y = s * a.y;

	return result;
}

inline float inner(v2 a, v2 b) {
	float result = a.x*b.x + a.y*b.y;

	return result;
}

inline rectangle2i Rect(int minX, int minY, int maxX, int maxY) {
	rectangle2i result;

	result.minX = minX;
	result.minY = minY;
	result.maxX = maxX;
	result.maxY = maxY;

	return result;
}

inline rectangle2i makeRectV2(v2 min, v2 max) {
	rectangle2i result;
	v2i minI = roundV2(min);
	v2i maxI = roundV2(max);

	result.minX = minI.x;
	result.minY = minI.y;
	result.maxX = maxI.x;
	result.maxY = maxI.y;

	return result;
}

inline rectangle2i clipRect(rectangle2i a, rectangle2i b) {
	rectangle2i result;

	result.minX = (a.minX < b.minX) ? b.minX : a.minX;
	result.minY = (a.minY < b.minY) ? b.minY : a.minY;
	result.maxX = (a.maxX > b.maxX) ? b.maxX : a.maxX;
	result.maxY = (a.maxY > b.maxY) ? b.maxY : a.maxY;

	return result;
}

#endif