#pragma once

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define kilobytes(value) ((value) * 1024LL)
#define megabytes(value) (kilobytes(value) * 1024LL)
#define gigabytes(value) (megabytes(value) * 1024LL)

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

enum wall {
	WALL_NONE,

	WALL_LEFT,
	WALL_UP,
	WALL_RIGHT,
	WALL_DOWN,
};

struct game_memory {
	void* storage;
	u32 storageSize;
};

struct player {
	v2 paddlePos;
	// Size is (width, height)
	v2 size;
	u32 score;

	v2 vertices[4];
};

struct ball {
	v2 ballPos;
	// Size is (width, height)
	v2 size;
	v2 velocity;

	v2 vertices[4];
};

struct game_state {
	player players[2];
	ball theBall;

	u32 arenaWidth, arenaHeight;

	v2 staticVertices[2];
};