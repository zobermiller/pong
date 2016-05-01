#include "precompiled.h"
#include "pong_math.h"
#include "win32_pong.h"

#define SDL 0
#define VSYNC 0

#if SDL
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>
#else
#define WIN32_LEAN_AND_MEAN
#endif

bool keyDown[256] = {0};
s64 globalPerfCountFrequency;

void handleKeyDown(int vkCode) {
	if(vkCode == VK_ESCAPE)
		PostQuitMessage(0);
	keyDown[vkCode] = true;
}

void handleKeyUp(int vkCode) {
	keyDown[vkCode] = false;
}

inline LARGE_INTEGER getWallClock() {
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

inline s64 getMicrosecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
	s64 result = (((end.QuadPart - start.QuadPart) * 1000000) / globalPerfCountFrequency);
	return result;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_KEYDOWN: {
			handleKeyDown((int)wParam);
		} break;

		case WM_KEYUP: {
			handleKeyUp((int)wParam);
		} break;

		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;

		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return NULL;
}

bool initGL() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, Screen_Width, Screen_Height, 0.0f, 1.0f, -1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		OutputDebugString("Error initializing OpenGL!\n");
		return false;
	}

	return true;
}

void makeRectFromCenterPoint(v2 vertices[], v2 centerPoint, v2 size) {
	vertices[0] = V2(centerPoint.x - 0.5f * size.x,
	                 centerPoint.y - 0.5f * size.y);
	vertices[1] = V2(centerPoint.x + 0.5f * size.x,
	                 centerPoint.y - 0.5f * size.y);
	vertices[2] = V2(centerPoint.x + 0.5f * size.x,
	                 centerPoint.y + 0.5f * size.y);
	vertices[3] = V2(centerPoint.x - 0.5f * size.x,
	                 centerPoint.y + 0.5f * size.y);
}

void initGameState(game_state* gameState, u32 arenaWidth, u32 arenaHeight,
                   v2 player1Pos, v2 player2Pos, v2 pos,
                   v2 playerSize, v2 ballSize) {
	gameState->arenaWidth = arenaWidth;
	gameState->arenaHeight = arenaHeight;

	gameState->players[0].pos = player1Pos;
	gameState->players[0].score = 0;
	gameState->players[0].size = playerSize;

	gameState->players[1].pos = player2Pos;
	gameState->players[1].score = 0;
	gameState->players[1].size = playerSize;

	gameState->ball.pos = pos;
	gameState->ball.size = ballSize;
	gameState->ball.velocity = V2(0, 0);

	gameState->programRunning = true;

	makeRectFromCenterPoint(gameState->ball.vertices, gameState->ball.pos, gameState->ball.size);
	makeRectFromCenterPoint(gameState->players[0].vertices, gameState->players[0].pos, 
	                        gameState->players[0].size);
	makeRectFromCenterPoint(gameState->players[1].vertices, gameState->players[1].pos, 
	                        gameState->players[1].size);
}

wall collidedWithWall(v2 pos, v2 size, u32 width, u32 height) {
	float xMin = pos.x - 0.5f * size.x;
	float xMax = pos.x + 0.5f * size.x;
	float yMin = pos.y - 0.5f * size.y;
	float yMax = pos.y + 0.5f * size.y;

	if(xMin < 0)
		return WallLeft;
	else if(yMin < 0)
		return WallUp;
	else if(xMax > width)
		return WallRight;
	else if(yMax > height)
		return WallDown;

	return WallNone;
}

void update(game_state* gameState, s64 microsecondsPerFrame) {
	float dt = microsecondsPerFrame * Microseconds_To_Milliseconds;
	wall whichWall = collidedWithWall(gameState->ball.pos, gameState->ball.size, gameState->arenaWidth, gameState->arenaHeight);

	if(whichWall == WallLeft || whichWall == WallRight)
		gameState->ball.velocity = V2(-gameState->ball.velocity.x, gameState->ball.velocity.y);
	if(whichWall == WallUp || whichWall == WallDown)
		gameState->ball.velocity = V2(gameState->ball.velocity.x, -gameState->ball.velocity.y);

	gameState->ball.pos += dt * gameState->ball.velocity;

	whichWall = collidedWithWall(gameState->players[0].pos, gameState->players[0].size, 
	                             gameState->arenaWidth, gameState->arenaHeight);
	v2 player1VelocityUp = V2(0, -200);
	v2 player1VelocityDown = V2(0, 200);

	if(whichWall == WallUp)
		player1VelocityUp = V2(0, 0);
	if(whichWall == WallDown)
		player1VelocityDown = V2(0, 0);

	whichWall = collidedWithWall(gameState->players[1].pos, gameState->players[1].size, 
	                             gameState->arenaWidth, gameState->arenaHeight);
	v2 player2VelocityUp = V2(0, -200);
	v2 player2VelocityDown = V2(0, 200);

	if(whichWall == WallUp)
		player2VelocityUp = V2(0, 0);
	if(whichWall == WallDown)
		player2VelocityDown = V2(0, 0);

#if !SDL
	if(keyDown[0x57]) // W
		gameState->players[0].pos += dt * player1VelocityUp;
	if(keyDown[0x53]) // S
		gameState->players[0].pos += dt * player1VelocityDown;

	if(keyDown[0x49]) // I
		gameState->players[1].pos += dt * player2VelocityUp;
	if(keyDown[0x4b]) // K
		gameState->players[1].pos += dt * player2VelocityDown;

	static bool spacePressed = false;
	if(keyDown[VK_SPACE] && spacePressed == false) {
		gameState->ball.velocity = Ball_Initial_Velocity;
		spacePressed = true;
	}
#else
	if(keyDown[SDL_SCANCODE_W])
		gameState->players[0].pos += dt * player1VelocityUp;
	if(keyDown[SDL_SCANCODE_S])
		gameState->players[0].pos += dt * player1VelocityDown;

	if(keyDown[SDL_SCANCODE_I])
		gameState->players[1].pos += dt * player2VelocityUp;
	if(keyDown[SDL_SCANCODE_K])
		gameState->players[1].pos += dt * player2VelocityDown;

	if(keyDown[SDL_SCANCODE_ESCAPE])
		gameState->programRunning = false;

	static bool spacePressed = false;
	if(keyDown[SDL_SCANCODE_SPACE] && spacePressed == false) {
		gameState->ball.velocity = Ball_Initial_Velocity;
		spacePressed = true;
	}
#endif
}

#if !SDL
void render(game_state* gameState) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	// Renders the center line of the board
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(Screen_Width / 2.0f, 0.0f);
	glVertex2f(Screen_Width / 2.0f, Screen_Height);
	glEnd();

	float yOffsetPlayer0 = Player_Default_Y - gameState->players[0].pos.y;
	float yOffsetPlayer1 = Player_Default_Y - gameState->players[1].pos.y;

	float xOffsetBall = Ball_Default_X - gameState->ball.pos.x;
	float yOffsetBall = Ball_Default_Y - gameState->ball.pos.y;

	glPushMatrix();
	glTranslatef(0.0f, -yOffsetPlayer0, 0.0f);
	glBegin(GL_QUADS);
	for(int i=0; i < 4; i++)
		glVertex2f(gameState->players[0].vertices[i].x, gameState->players[0].vertices[i].y);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -yOffsetPlayer1, 0.0f);
	glBegin(GL_QUADS);
	for(int i=0; i < 4; i++)
		glVertex2f(gameState->players[1].vertices[i].x, gameState->players[1].vertices[i].y);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-xOffsetBall, -yOffsetBall, 0.0f);
	glBegin(GL_QUADS);
	for(int i=0; i < 4; i++)
		glVertex2f(gameState->ball.vertices[i].x, gameState->ball.vertices[i].y);
	glEnd();
	glPopMatrix();

	glFlush();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, s32 nCmdShow) {
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	globalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

	UINT desiredSchedulerMS = 1;
	bool sleepIsGranular = (timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR);

	WNDCLASSEX wc;

	wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);

	HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW,
	                           "WindowClass",
	                           "Pong",
	                           WS_OVERLAPPEDWINDOW,
	                           0, 0,
	                           Screen_Width, Screen_Height,
	                           NULL,
	                           NULL,
	                           hInstance,
	                           NULL);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		1,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	HDC deviceContext = GetDC(hWnd);

	s32 pixelFormat = ChoosePixelFormat(deviceContext, &pfd);
	SetPixelFormat(deviceContext, pixelFormat, &pfd);

	HGLRC renderContext = wglCreateContext(deviceContext);
	wglMakeCurrent(deviceContext, renderContext);

	ShowWindow(hWnd, nCmdShow);

	game_memory gameMemory = {};
	gameMemory.storageSize = megabytes(1);
	gameMemory.storage = VirtualAlloc(0, (size_t)gameMemory.storageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	game_state *gameState = (game_state*)gameMemory.storage;

	initGL();
	initGameState(gameState, Screen_Width, Screen_Height, V2(50, Player_Default_Y), 
	              V2(Screen_Width - 50, Player_Default_Y), V2(Ball_Default_X, Ball_Default_Y), 
	              V2(Player_Width, Player_Height), V2(Ball_Width, Ball_Height));

	bool running = true;
	MSG msg;
	LARGE_INTEGER endCounter = getWallClock();
	s32 targetMillisecondsPerFrame = 16;
	s32 targetMicrosecondsPerFrame = 16666;

	PFNWGLSWAPINTERVALEXTPROC proc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
#if VSYNC
	proc(1);
#else
	proc(0);
#endif
	while(running) {
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			switch(msg.message) {
				case WM_QUIT: {
					running = false;
				} break;

				default: {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				} break;
			}
		}
#if VSYNC
		update(gameState, targetMicrosecondsPerFrame);
		render(gameState);
		SwapBuffers(deviceContext);
#else
		LARGE_INTEGER workCounter = getWallClock();
		s64 microsecondsElapsed = getMicrosecondsElapsed(endCounter, workCounter);

		update(gameState, microsecondsElapsed);
		render(gameState);
		SwapBuffers(deviceContext);

		endCounter = workCounter;

		/*if(microsecondsElapsed < targetMicrosecondsPerFrame) {
			DWORD sleepMS = (DWORD)(1000 * (targetSecondsPerFrame - microsecondsElapsed));
			if(sleepMS > 0) {
				Sleep(0);
			}

			while(microsecondsElapsed < targetMicrosecondsPerFrame) {
				microsecondsElapsed = getMicrosecondsElapsed(endCounter, getWallClock());
			}
		}*/
#endif
	}

	wglDeleteContext(renderContext);
	VirtualFree(gameMemory.storage, 0, MEM_RELEASE);
	return (int)msg.wParam;
}
#else

void renderSDL(game_state* gameState, SDL_Renderer* renderer) {
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	s32 player0X = (s32)round(gameState->players[0].pos.x - (Player_Width / 2));
	s32 player0Y = (s32)round(gameState->players[0].pos.y - (Player_Height /2));
	s32 player1X = (s32)round(gameState->players[1].pos.x - (Player_Width / 2));
	s32 player1Y = (s32)round(gameState->players[1].pos.y - (Player_Height / 2));

	s32 ballX = (s32)round(gameState->ball.pos.x - (Ball_Width / 2));
	s32 ballY = (s32)round(gameState->ball.pos.y - (Ball_Height /2));

	SDL_Rect player0Rect = {player0X, player0Y, Player_Width, Player_Height};
	SDL_Rect player1Rect = {player1X, player1Y, Player_Width, Player_Height};
	SDL_Rect ballRect = {ballX, ballY, Ball_Width, Ball_Height};

	SDL_RenderFillRect(renderer, &player0Rect);
	SDL_RenderFillRect(renderer, &player1Rect);
	SDL_RenderFillRect(renderer, &ballRect);

	SDL_RenderDrawLine(renderer, Screen_Width / 2, 0, Screen_Width / 2, Screen_Height);

	SDL_RenderPresent(renderer);
}

void sdlHandleKeys(const u8* keys) {
	for(int i=0; i < 256; i++) {
		if(keys[i])
			keyDown[i] = true;
		else
			keyDown[i] = false;
	}
}

int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	SDL_Window *window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                                      Screen_Width, Screen_Height, SDL_WINDOW_SHOWN);
#if VSYNC
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
#else
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
#endif
	game_memory gameMemory = {};
	gameMemory.storageSize = megabytes(1);
	gameMemory.storage = VirtualAlloc(0, (size_t)gameMemory.storageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	game_state* gameState = (game_state*)gameMemory.storage;
	initGameState(gameState, Screen_Width, Screen_Height, V2(50, Player_Default_Y), 
	              V2(Screen_Width - 50, Player_Default_Y), V2(Ball_Default_X, Ball_Default_Y), 
	              V2(Player_Width, Player_Height), V2(Ball_Width, Ball_Height));

	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	globalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

	SDL_Event e;
	s64 targetMicrosecondsPerFrame = 16666;
	LARGE_INTEGER lastCounter = getWallClock();

	while(gameState->programRunning) {
		if(SDL_PollEvent(&e)) {
			if(e.type == SDL_QUIT)
				gameState->programRunning = false;
		}
		const u8* keyEvents = SDL_GetKeyboardState(NULL);
		sdlHandleKeys(keyEvents);
#if VSYNC
		update(gameState, targetMicrosecondsPerFrame);
		renderSDL(gameState, renderer);
#else
		LARGE_INTEGER workCounter = getWallClock();
		s64 microsecondsElapsed = getMicrosecondsElapsed(lastCounter, workCounter);

		update(gameState, microsecondsElapsed);
		renderSDL(gameState, renderer);

		lastCounter = workCounter;
#endif
	}

	VirtualFree(gameMemory.storage, 0, MEM_RELEASE);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}
#endif