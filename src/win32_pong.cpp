#include "precompiled.h"
#include "pong_math.h"
#include "win32_pong.h"

#define VSYNC 0

bool keyDown[256] = {0};

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

inline s64 getMicrosecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end, s64 perfCountFrequency) {
	s64 result = (((end.QuadPart - start.QuadPart) * 1000000)  / perfCountFrequency);
	return result;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;

		case WM_KEYDOWN: {
			handleKeyDown((int)wParam);
		} break;

		case WM_KEYUP: {
			handleKeyUp((int)wParam);
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

inline void makeRectFromCenterPoint(v2 vertices[], v2 centerPoint, v2 size) {
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

inline wall collidedWithWall(v2 pos, v2 size, u32 width, u32 height) {
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

inline void update(game_state* gameState, float targetMilliseconds) {
	float dt = targetMilliseconds / 1000.0f;
	wall whichWall = collidedWithWall(gameState->ball.pos, gameState->ball.size, 
	                                  gameState->arenaWidth, gameState->arenaHeight);

	if(whichWall == WallLeft || whichWall == WallRight) {
		gameState->ball.velocity = V2(-gameState->ball.velocity.x, gameState->ball.velocity.y);
	}
	if(whichWall == WallUp || whichWall == WallDown) {
		gameState->ball.velocity = V2(gameState->ball.velocity.x, -gameState->ball.velocity.y);
	}

	gameState->ball.pos += dt * gameState->ball.velocity;

	whichWall = collidedWithWall(gameState->players[0].pos, gameState->players[0].size, 
	                             gameState->arenaWidth, gameState->arenaHeight);
	v2 player1VelocityUp = V2(0, -200);
	v2 player1VelocityDown = V2(0, 200);

	if(whichWall == WallUp) {
		player1VelocityUp = V2(0, 0);
	}
	if(whichWall == WallDown) {
		player1VelocityDown = V2(0, 0);
	}

	whichWall = collidedWithWall(gameState->players[1].pos, gameState->players[1].size, 
	                             gameState->arenaWidth, gameState->arenaHeight);
	v2 player2VelocityUp = V2(0, -200);
	v2 player2VelocityDown = V2(0, 200);

	if(whichWall == WallUp) {
		player2VelocityUp = V2(0, 0);
	}
	if(whichWall == WallDown) {
		player2VelocityDown = V2(0, 0);
	}

	if(keyDown[0x57]) { // W
		gameState->players[0].pos += dt * player1VelocityUp;
	}
	if(keyDown[0x53]) { // S
		gameState->players[0].pos += dt * player1VelocityDown;
	}

	if(keyDown[0x49]) { // I
		gameState->players[1].pos += dt * player2VelocityUp;
	}
	if(keyDown[0x4b]) { // K
		gameState->players[1].pos += dt * player2VelocityDown;
	}
	if(keyDown[VK_SPACE]) {
		gameState->ball.velocity = Ball_Initial_Velocity;
	}
}

inline void gLQuad(v2 vertices[]) {
	glBegin(GL_QUADS);

	glVertex2f(vertices[0].x, vertices[0].y);
	glVertex2f(vertices[1].x, vertices[1].y);
	glVertex2f(vertices[2].x, vertices[2].y);
	glVertex2f(vertices[3].x, vertices[3].y);

	glEnd();
}

inline void gLLine(float x0, float y0, float x1, float y1) {
	glBegin(GL_LINES);

	glVertex2f(x0, y0);
	glVertex2f(x1, y1);

	glEnd();
}

inline void render(game_state* gameState, float offset) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	offset /= 1000.0f;
	v2 ballOffset = offset * V2(gameState->ball.velocity.x, gameState->ball.velocity.y);
	makeRectFromCenterPoint(gameState->ball.vertices, gameState->ball.pos + ballOffset, gameState->ball.size);
	makeRectFromCenterPoint(gameState->players[0].vertices, gameState->players[0].pos, 
	                        gameState->players[0].size);
	makeRectFromCenterPoint(gameState->players[1].vertices, gameState->players[1].pos, 
	                        gameState->players[1].size);

	// Renders the center line of the board
	glColor3f(1.0f, 1.0f, 1.0f);
	gLLine(Screen_Width / 2.0f, 0.0f, Screen_Width / 2.0f, Screen_Height);

	gLQuad(gameState->players[0].vertices);
	gLQuad(gameState->players[1].vertices);
	gLQuad(gameState->ball.vertices);

	glFlush();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, s32 nCmdShow) {
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	s64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

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

	MSG msg;
	LARGE_INTEGER previous = getWallClock();
	float lag = 0.0f;
	float targetMS = 1 / 60.0f;
	s64 targetMicroseconds = 16666;

	PFNWGLSWAPINTERVALEXTPROC proc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
#if VSYNC
	proc(-1);
#else
	proc(0);
#endif
	while(gameState->programRunning) {
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			switch(msg.message) {
				case WM_QUIT: {
					gameState->programRunning = false;
				} break;

				default: {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				} break;
			}
		}
#if VSYNC
		update(gameState, targetMS);
		render(gameState);
		SwapBuffers(deviceContext);
#else
		LARGE_INTEGER current = getWallClock();
		s64 microsecondsElapsed = getMicrosecondsElapsed(previous, current, perfCountFrequency);
		previous = current;
		lag += (microsecondsElapsed / 1000.0f);

		while(lag >= targetMS) {
			update(gameState, targetMS);
			lag -= targetMS;
		}

		float offset = lag / targetMS;
		render(gameState, offset);
		SwapBuffers(deviceContext);

		// LARGE_INTEGER sleep = getWallClock();
		// s64 remaining = getMicrosecondsElapsed(current, sleep, perfCountFrequency);
		// if(remaining < targetMicroseconds) {
		// 	s64 amount = (targetMicroseconds - remaining) / 1000;
		// 	Sleep((DWORD)amount);
		// }
#endif
	}

	wglDeleteContext(renderContext);
	VirtualFree(gameMemory.storage, 0, MEM_RELEASE);
	return (int)msg.wParam;
}
