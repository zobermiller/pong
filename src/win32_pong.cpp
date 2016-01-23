#include "precompiled.h"
#include "pong_math.h"
#include "win32_pong.h"

#define SDL 1

#if SDL
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_ttf.h>
#endif

bool keyDown[256];
s64 globalPerfCountFrequency;

void handleKeyDown(int vkCode) {
	if(vkCode == VK_ESCAPE)
		PostQuitMessage(0);
	if(vkCode >= 0x30 || vkCode <= 0x5a)
		keyDown[vkCode] = true;
}

void handleKeyUp(int vkCode) {
	if(vkCode >= 0x30 || vkCode <= 0x5a)
		keyDown[vkCode] = false;
}

LARGE_INTEGER getWallClock() {
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

float getSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
	float result = ((float)(end.QuadPart - start.QuadPart) / (float)globalPerfCountFrequency);
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, -1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		OutputDebugString("Error initializing OpenGL!\n");
		return false;
	}

	return true;
}

void makeRectFromCenterPoint(v2 centerPoint, v2 size, v2 vertices[]) {
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

	gameState->centerLine[0] = V2(SCREEN_WIDTH / 2, 0);
	gameState->centerLine[1] = V2(SCREEN_WIDTH / 2, SCREEN_HEIGHT);

	gameState->programRunning = true;

	makeRectFromCenterPoint(gameState->ball.pos, gameState->ball.size, gameState->ball.vertices);
	makeRectFromCenterPoint(gameState->players[0].pos, gameState->players[0].size, gameState->players[0].vertices);
	makeRectFromCenterPoint(gameState->players[1].pos, gameState->players[1].size, gameState->players[1].vertices);

	for(int i=0; i < 256; i++)
		keyDown[i] = false;
}

wall collidedWithWall(v2 pos, v2 size, u32 width, u32 height) {
	float xMin = pos.x - 0.5f * size.x;
	float xMax = pos.x + 0.5f * size.x;
	float yMin = pos.y - 0.5f * size.y;
	float yMax = pos.y + 0.5f * size.y;

	if(xMin < 0)
		return WALL_LEFT;
	else if(yMin < 0)
		return WALL_UP;
	else if(xMax > width)
		return WALL_RIGHT;
	else if(yMax > height)
		return WALL_DOWN;

	return WALL_NONE;
}

void update(game_state* gameState, float dt) {
	wall whichWall = collidedWithWall(gameState->ball.pos, gameState->ball.size, gameState->arenaWidth, gameState->arenaHeight);

	if(whichWall == WALL_LEFT || whichWall == WALL_RIGHT)
		gameState->ball.velocity = V2(-gameState->ball.velocity.x, gameState->ball.velocity.y);
	if(whichWall == WALL_UP || whichWall == WALL_DOWN)
		gameState->ball.velocity = V2(gameState->ball.velocity.x, -gameState->ball.velocity.y);

	gameState->ball.pos += dt * gameState->ball.velocity;

	whichWall = collidedWithWall(gameState->players[0].pos, gameState->players[0].size, gameState->arenaWidth, gameState->arenaHeight);
	v2 player1VelocityUp = V2(0, -200);
	v2 player1VelocityDown = V2(0, 200);

	if(whichWall == WALL_UP)
		player1VelocityUp = V2(0, 0);
	if(whichWall == WALL_DOWN)
		player1VelocityDown = V2(0, 0);

	whichWall = collidedWithWall(gameState->players[1].pos, gameState->players[1].size, gameState->arenaWidth, gameState->arenaHeight);
	v2 player2VelocityUp = V2(0, -200);
	v2 player2VelocityDown = V2(0, 200);

	if(whichWall == WALL_UP)
		player2VelocityUp = V2(0, 0);
	if(whichWall == WALL_DOWN)
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
		gameState->ball.velocity = V2(600.0f, 0.0f);
		spacePressed = true;
	}
#endif
}

#if !SDL
void render(game_state* gameState) {
	glEnable(GL_MULT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(gameState->centerLine[0].x, gameState->centerLine[0].y);
	glVertex2f(gameState->centerLine[1].x, gameState->centerLine[1].y);
	glEnd();

	float yOffset_player0 = PLAYER_DEFAULT_Y - gameState->players[0].pos.y;
	float yOffset_player1 = PLAYER_DEFAULT_Y - gameState->players[1].pos.y;

	float xOffset_ball = BALL_DEFAULT_X - gameState->ball.pos.x;
	float yOffset_ball = BALL_DEFAULT_Y - gameState->ball.pos.y;

	glPushMatrix();
	glTranslatef(0.0f, -yOffset_player0, 0.0f);
	glBegin(GL_QUADS);
	for(int i=0; i < 4; i++)
		glVertex2f(gameState->players[0].vertices[i].x, gameState->players[0].vertices[i].y);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, -yOffset_player1, 0.0f);
	glBegin(GL_QUADS);
	for(int i=0; i < 4; i++)
		glVertex2f(gameState->players[1].vertices[i].x, gameState->players[1].vertices[i].y);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-xOffset_ball, -yOffset_ball, 0.0f);
	glBegin(GL_QUADS);
	for(int i=0; i < 4; i++)
		glVertex2f(gameState->ball.vertices[i].x, gameState->ball.vertices[i].y);
	glEnd();
	glPopMatrix();

	glFlush();
	glDisable(GL_MULT);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;

	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	globalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

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
												WS_OVERLAPPEDWINDOW,// | WS_CAPTION | WS_SYSMENU |
												//WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
												0, 0,
												SCREEN_WIDTH, SCREEN_HEIGHT,
												NULL,
												NULL,
												hInstance,
												NULL);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		32,                        //Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		1,
		0,
		0,
		0, 0, 0, 0,
		16,                        //Number of bits for the depthbuffer
		0,                        //Number of bits for the stencilbuffer
		0,                        //Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	HDC deviceContext = GetDC(hWnd);

	int pixelFormat = ChoosePixelFormat(deviceContext, &pfd);
	SetPixelFormat(deviceContext, pixelFormat, &pfd);

	HGLRC renderContext = wglCreateContext(deviceContext);
	wglMakeCurrent(deviceContext, renderContext);

	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoose = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

	int iAttributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 24,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 16,
		WGL_STENCIL_BITS_ARB, 0,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0, 0
	};
	float fAttributes[] = {0, 0};
	int format;
	u32 numFormats;

	int valid = wglChoose(deviceContext, iAttributes, fAttributes, 1, &format, &numFormats);

	/*wglMakeCurrent(deviceContext, 0);
	wglDeleteContext(renderContext);
	renderContext = 0;
	ReleaseDC(hWnd, deviceContext);
	deviceContext = 0;
	DestroyWindow(hWnd);
	hWnd = 0;*/

	hWnd = CreateWindowEx(WS_EX_APPWINDOW,
												"WindowClass",
												"Pong",
												WS_OVERLAPPEDWINDOW,// | WS_CAPTION | WS_SYSMENU |
												//WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
												0, 0,
												SCREEN_WIDTH, SCREEN_HEIGHT,
												NULL,
												NULL,
												hInstance,
												NULL);
	deviceContext = GetDC(hWnd);
	SetPixelFormat(deviceContext, format, &pfd);
	renderContext = wglCreateContext(deviceContext);
	wglMakeCurrent(deviceContext, renderContext);

	ShowWindow(hWnd, nCmdShow);

	game_memory gameMemory = {};
	gameMemory.storageSize = megabytes(1);
	gameMemory.storage = VirtualAlloc(0, (size_t)gameMemory.storageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	game_state *gameState = (game_state*)gameMemory.storage;

	initGL();
	initGameState(gameState, SCREEN_WIDTH, SCREEN_HEIGHT, V2(50, PLAYER_DEFAULT_Y), V2(SCREEN_WIDTH - 50, PLAYER_DEFAULT_Y),
								V2(BALL_DEFAULT_X, BALL_DEFAULT_Y), V2(PLAYER_WIDTH, PLAYER_HEIGHT), V2(BALL_WIDTH, BALL_HEIGHT));


	bool running = true;
	MSG msg;
	LARGE_INTEGER lastCounter = getWallClock();
	float targetSecondsPerFrame = 1.0f / 60.0f;

	PFNWGLSWAPINTERVALEXTPROC proc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if(proc)
		proc(0);

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

		LARGE_INTEGER workCounter = getWallClock();
		float workSecondsElapsed = getSecondsElapsed(lastCounter, workCounter);
		float secondsElapsedForFrame = workSecondsElapsed;

		update(gameState, targetSecondsPerFrame);
		render(gameState);
		SwapBuffers(deviceContext);

		if(secondsElapsedForFrame < targetSecondsPerFrame) {
			DWORD sleepMS = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
			if(sleepMS > 0) {
				Sleep(0);
			}

			while(secondsElapsedForFrame < targetSecondsPerFrame) {
				secondsElapsedForFrame = getSecondsElapsed(lastCounter, getWallClock());
			}
		}

		LARGE_INTEGER endCounter = getWallClock();
		lastCounter = endCounter;
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

	s32 player0X = (s32)(gameState->players[0].pos.x - (PLAYER_WIDTH / 2));
	s32 player0Y = (s32)(gameState->players[0].pos.y - (PLAYER_HEIGHT /2));
	s32 player1X = (s32)(gameState->players[1].pos.x - (PLAYER_WIDTH / 2));
	s32 player1Y = (s32)(gameState->players[1].pos.y - (PLAYER_HEIGHT / 2));

	s32 ballX = (s32)(gameState->ball.pos.x - (BALL_WIDTH / 2));
	s32 ballY = (s32)(gameState->ball.pos.y - (BALL_HEIGHT /2));
	
	SDL_Rect player0Rect = {player0X, player0Y, PLAYER_WIDTH, PLAYER_HEIGHT};
	SDL_Rect player1Rect = {player1X, player1Y, PLAYER_WIDTH, PLAYER_HEIGHT};
	SDL_Rect ballRect = {ballX, ballY, BALL_WIDTH, BALL_HEIGHT};

	SDL_RenderFillRect(renderer, &player0Rect);
	SDL_RenderFillRect(renderer, &player1Rect);
	SDL_RenderFillRect(renderer, &ballRect);

	SDL_RenderDrawLine(renderer, (int)gameState->centerLine[0].x, (int)gameState->centerLine[0].y, (int)gameState->centerLine[1].x, (int)gameState->centerLine[1].y);
	
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

int main(int argv, char** argc) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	SDL_Window *window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
																				SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	TTF_Init();
	
	game_memory gameMemory = {};
	gameMemory.storageSize = megabytes(1);
	gameMemory.storage = malloc(gameMemory.storageSize);

	game_state* gameState = (game_state*)gameMemory.storage;
	initGameState(gameState, SCREEN_WIDTH, SCREEN_HEIGHT, V2(50, PLAYER_DEFAULT_Y), V2(SCREEN_WIDTH - 50, PLAYER_DEFAULT_Y),
								V2(BALL_DEFAULT_X, BALL_DEFAULT_Y), V2(PLAYER_WIDTH, PLAYER_HEIGHT), V2(BALL_WIDTH, BALL_HEIGHT));

	SDL_Event e;
	float frameSeconds = 1.0f / 60.0f;
	while(gameState->programRunning) {
		if(SDL_PollEvent(&e)) {
			if(e.type == SDL_QUIT)
				gameState->programRunning = false;
		}
		const u8* keyEvents = SDL_GetKeyboardState(NULL);
		sdlHandleKeys(keyEvents);

		update(gameState, frameSeconds);
		renderSDL(gameState, renderer);
	}

	free(gameMemory.storage);

	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}
#endif