#ifndef PONG_MATH_H
#define PONG_MATH_H

struct v2 {
	float x, y;
};

inline v2 operator+(v2 a, v2 b) {
	v2 result;
	
	result.x = a.x + b.x;
	result.y = a.y + b.y;

	return result;
}

inline v2 operator-(v2 a, v2 b) {
	v2 result;

	result.x = a.x - b.x;
	result.y = a.y - b.y;

	return result;
}

inline float inner(v2 a, v2 b) {
	float result = a.x*b.x + a.y*b.y;

	return result;
}

#endif