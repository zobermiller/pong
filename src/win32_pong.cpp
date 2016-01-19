#include "precompiled.h"
#include "pong_math.h"
#include "win32_pong.h"

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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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
									 v2 player1Pos, v2 player2Pos, v2 ballPos,
									 v2 playerSize, v2 ballSize) {
	gameState->arenaWidth = arenaWidth;
	gameState->arenaHeight = arenaHeight;

	gameState->players[0].paddlePos = player1Pos;
	gameState->players[0].score = 0;
	gameState->players[0].size = playerSize;

	gameState->players[1].paddlePos = player2Pos;
	gameState->players[1].score = 0;
	gameState->players[1].size = playerSize;

	gameState->theBall.ballPos = ballPos;
	gameState->theBall.size = ballSize;
	gameState->theBall.velocity = V2(600, 0);

	gameState->staticVertices[0] = V2(SCREEN_WIDTH / 2, 0);
	gameState->staticVertices[1] = V2(SCREEN_WIDTH / 2, SCREEN_HEIGHT);
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
	wall whichWall = collidedWithWall(gameState->theBall.ballPos, gameState->theBall.size, gameState->arenaWidth, gameState->arenaHeight);

	if(whichWall == WALL_LEFT || whichWall == WALL_RIGHT)
		gameState->theBall.velocity = V2(-gameState->theBall.velocity.x, gameState->theBall.velocity.y);
	if(whichWall == WALL_UP || whichWall == WALL_DOWN)
		gameState->theBall.velocity = V2(gameState->theBall.velocity.x, -gameState->theBall.velocity.y);

	gameState->theBall.ballPos += dt * gameState->theBall.velocity;

	whichWall = collidedWithWall(gameState->players[0].paddlePos, gameState->players[0].size, gameState->arenaWidth, gameState->arenaHeight);
	v2 player1VelocityUp = V2(0, -200);
	v2 player1VelocityDown = V2(0, 200);

	if(whichWall == WALL_UP)
		player1VelocityUp = V2(0, 0);
	if(whichWall == WALL_DOWN)
		player1VelocityDown = V2(0, 0);

	whichWall = collidedWithWall(gameState->players[1].paddlePos, gameState->players[1].size, gameState->arenaWidth, gameState->arenaHeight);
	v2 player2VelocityUp = V2(0, -200);
	v2 player2VelocityDown = V2(0, 200);

	if(whichWall == WALL_UP)
		player2VelocityUp = V2(0, 0);
	if(whichWall == WALL_DOWN)
		player2VelocityDown = V2(0, 0);

	if(keyDown[0x57]) // W
		gameState->players[0].paddlePos += dt * player1VelocityUp;
	if(keyDown[0x53]) // S
		gameState->players[0].paddlePos += dt * player1VelocityDown;

	if(keyDown[0x49]) // I
		gameState->players[1].paddlePos += dt * player2VelocityUp;
	if(keyDown[0x4b]) // K
		gameState->players[1].paddlePos += dt * player2VelocityDown;

	makeRectFromCenterPoint(gameState->theBall.ballPos, gameState->theBall.size, gameState->theBall.vertices);
	makeRectFromCenterPoint(gameState->players[0].paddlePos, gameState->players[0].size, gameState->players[0].vertices);
	makeRectFromCenterPoint(gameState->players[1].paddlePos, gameState->players[1].size, gameState->players[1].vertices);
}

void render(game_state* gameState) {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(gameState->staticVertices[0].x, gameState->staticVertices[0].y);
	glVertex2f(gameState->staticVertices[1].x, gameState->staticVertices[1].y);
	glEnd();

	glBegin(GL_QUADS);
	for(int i=0; i < 2; i++) {
		for(int j=0; j < 4; j++)
			glVertex2f(gameState->players[i].vertices[j].x, gameState->players[i].vertices[j].y);
	}

	for(int i=0; i < 4; i++)
		glVertex2f(gameState->theBall.vertices[i].x, gameState->theBall.vertices[i].y);
	glEnd();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;
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

	hWnd = CreateWindowEx(NULL,
												"WindowClass",
												"Pong",
												WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
												WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
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
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                        //Number of bits for the depthbuffer
		8,                        //Number of bits for the stencilbuffer
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

	ShowWindow(hWnd, nCmdShow);

	game_memory gameMemory = {};
	gameMemory.storageSize = megabytes(1);
	gameMemory.storage = VirtualAlloc(0, (size_t)gameMemory.storageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	game_state *gameState = (game_state*)gameMemory.storage;

	initGL();
	initGameState(gameState, SCREEN_WIDTH, SCREEN_HEIGHT, V2(50, SCREEN_HEIGHT / 2), V2(SCREEN_WIDTH - 50, SCREEN_HEIGHT / 2),
								V2(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2), V2(20, 50), V2(10, 10));

	bool running = true;
	MSG msg;
	LARGE_INTEGER lastCounter = getWallClock();
	float targetSecondsPerFrame = 1.0f / 60.0f;

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