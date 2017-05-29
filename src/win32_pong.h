#ifndef WIN32_PONG_H
#define WIN32_PONG_H

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <cmath>
#include <gl/gl.h>
#include <gl/wglext.h>

#include "pong_math.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define kilobytes(value) ((value) * 1024LL)
#define megabytes(value) (kilobytes(value) * 1024LL)
#define gigabytes(value) (megabytes(value) * 1024LL)

#define Screen_Width 1280
#define Screen_Height 720

#define Ball_Width 10
#define Ball_Height 10
#define Player_Width 20
#define Player_Height 50

#define Player_Default_Y (Screen_Height / 2.0f)

#define Paddle_Velocity_Up V2(0.0f, -400.0f)
#define Paddle_Velocity_Down V2(0.0f, 400.0f)

#define Ball_Initial_Velocity V2(1600.0f, 0.0f)

#define Ball_Default_X (Screen_Width / 2.0f)
#define Ball_Default_Y (Screen_Height / 2.0f)

#define Align16(value) (((value) + 15) & ~15)

enum wall {
	WallNone,

	WallLeft,
	WallRight,
	WallUp,
	WallDown,
};

struct offscreen_buffer {
	void* memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
	BITMAPINFO info;
};

struct game_memory {
	void* storage;
	u32 storageSize;
};

struct player {
	v2 pos;
	
	// Size is (width, height)
	v2 size; 
	u32 score;

	v2 vertices[4];
};

struct ball {
	v2 pos;

	// Size is (width, height)
	v2 size; 
	v2 velocity;

	v2 vertices[4];
};

struct button_state {
	bool endedDown;
};

struct program_input {
	button_state up;
	button_state down;
};

struct game_state {
	player players[2];
	program_input input[2];
	ball ball;

	u32 arenaWidth, arenaHeight;

	bool programRunning;
};

struct window_dimension {
	int width;
	int height;
};

#endif