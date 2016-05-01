#ifndef WIN32_PONG_H
#define WIN32_PONG_H

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

#define Screen_Width 1280
#define Screen_Height 720

#define Ball_Width 10
#define Ball_Height 10
#define Player_Width 20
#define Player_Height 50

#define Player_Default_Y Screen_Height / 2.0f

#define Ball_Initial_Velocity V2(600.0f, 0.0f)

#define Ball_Default_X Screen_Width / 2.0f
#define Ball_Default_Y Screen_Height / 2.0f

#define Microseconds_To_Milliseconds 1 / 1000000.0f

enum wall {
	WallNone,

	WallLeft,
	WallRight,
	WallUp,
	WallDown,
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

struct game_state {
	player players[2];
	ball ball;

	u32 arenaWidth, arenaHeight;

	bool programRunning;
};

#endif