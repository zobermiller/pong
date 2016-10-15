#include "win32_pong.h"

HWND hWnd;
static WINDOWPLACEMENT globalWindowPosition = { sizeof(globalWindowPosition) };

#define SOFTWARE 0
#define VSYNC 0

void toggleFullscreen(HWND window) {
	DWORD style = GetWindowLong(window, GWL_STYLE);

	if(style & WS_OVERLAPPEDWINDOW) {
		MONITORINFO monitorInfo = { sizeof(monitorInfo) };

		if(GetWindowPlacement(window, &globalWindowPosition) && GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
			SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(window, HWND_TOP,
			             monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
			             monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			             monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else {
		SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(window, &globalWindowPosition);
		SetWindowPos(window, 0, 0, 0, 0, 0,
		             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

void processKeyboardMessage(button_state *state, bool isDown) {
	if(state->endedDown != isDown) {
		state->endedDown = isDown;
	}
}

void processPendingMessages(game_state *gameState) {
	MSG msg;
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		switch(msg.message) {
			case WM_QUIT: {
				gameState->programRunning = false;
			} break;

			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN: {
				uint32_t vkCode = (uint32_t)msg.wParam;

				bool wasDown = ((msg.lParam & (1 << 30)) != 0);
				bool isDown = ((msg.lParam & (1 << 31)) == 0);
				if(wasDown != isDown) {
					if(vkCode == 'W') {
						processKeyboardMessage(&gameState->input[0].up, isDown);
					}
					else if(vkCode == 'S') {
						processKeyboardMessage(&gameState->input[0].down, isDown);
					}
					else if(vkCode == 'I') {
						processKeyboardMessage(&gameState->input[1].up, isDown);
					}
					else if(vkCode == 'K') {
						processKeyboardMessage(&gameState->input[1].down, isDown);
					}
					else if(vkCode == VK_ESCAPE) {
						PostQuitMessage(0);
					}
					if(isDown) {
						int altKey = (msg.lParam & (1 << 29));
						if((vkCode == VK_RETURN) && altKey) {
							toggleFullscreen(hWnd);
						}
					}
				}
			} break;

			default: {
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			} break;
		}
	}
}

inline LARGE_INTEGER getWallClock() {
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

inline uint64_t getMicrosecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end, uint64_t perfCountFrequency) {
	uint64_t result = (((end.QuadPart - start.QuadPart) * 1000000) / perfCountFrequency);
	return result;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hWnd, message, wParam, lParam);
}

window_dimension getWindowDimension(HWND hWnd) {
	window_dimension result;

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;

	return result;
}

inline void makeRectFromCenterPoint(v2 vertices[], v2 centerPoint, v2 size) {
	vertices[0] = V2(centerPoint.x - (0.5f * size.x), centerPoint.y - (0.5f * size.y));
	vertices[1] = V2(centerPoint.x + (0.5f * size.x), centerPoint.y - (0.5f * size.y));
	vertices[2] = V2(centerPoint.x + (0.5f * size.x), centerPoint.y + (0.5f * size.y));
	vertices[3] = V2(centerPoint.x - (0.5f * size.x), centerPoint.y + (0.5f * size.y));
}

void initGameState(game_state *gameState, uint32_t arenaWidth, uint32_t arenaHeight,
                   v2 player1Pos, v2 player2Pos, v2 pos, v2 playerSize, v2 ballSize) {
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
}

inline wall collidedWithWall(v2 pos, v2 size, uint32_t arenaWidth, uint32_t arenaHeight) {
	float xMin = pos.x - 0.5f * size.x;
	float xMax = pos.x + 0.5f * size.x;
	float yMin = pos.y - 0.5f * size.y;
	float yMax = pos.y + 0.5f * size.y;

	if(xMin < 0)
		return WallLeft;
	else if(yMin < 0)
		return WallUp;
	else if(xMax > arenaWidth)
		return WallRight;
	else if(yMax > arenaHeight)
		return WallDown;

	return WallNone;
}

void update(game_state *gameState, float dt) {
	wall whichWallBall = collidedWithWall(gameState->ball.pos, gameState->ball.size,
	                                      gameState->arenaWidth, gameState->arenaHeight);

	if(whichWallBall == WallLeft || whichWallBall == WallRight) {
		gameState->ball.velocity = V2(-gameState->ball.velocity.x, gameState->ball.velocity.y);
	}
	if(whichWallBall == WallUp || whichWallBall == WallDown) {
		gameState->ball.velocity = V2(gameState->ball.velocity.x, -gameState->ball.velocity.y);
	}

	v2 acceleration = V2(200.0f, 500.0f);
	gameState->ball.velocity += dt * acceleration;
	gameState->ball.pos += dt * gameState->ball.velocity;

	wall whichWallPlayer0 = collidedWithWall(gameState->players[0].pos, gameState->players[0].size,
	                                         gameState->arenaWidth, gameState->arenaHeight);
	v2 player0VelocityUp = Paddle_Velocity_Up;
	v2 player0VelocityDown = Paddle_Velocity_Down;

	if(whichWallPlayer0 == WallUp) {
		player0VelocityUp = V2(0, 0);
	}
	if(whichWallPlayer0 == WallDown) {
		player0VelocityDown = V2(0, 0);
	}

	wall whichWallPlayer1 = collidedWithWall(gameState->players[1].pos, gameState->players[1].size,
	                                         gameState->arenaWidth, gameState->arenaHeight);
	v2 player1VelocityUp = Paddle_Velocity_Up;
	v2 player1VelocityDown = Paddle_Velocity_Down;

	if(whichWallPlayer1 == WallUp) {
		player1VelocityUp = V2(0, 0);
	}
	if(whichWallPlayer1 == WallDown) {
		player1VelocityDown = V2(0, 0);
	}


	if(gameState->input[0].up.endedDown) {
		gameState->players[0].pos += dt * player0VelocityUp;
	}
	if(gameState->input[0].down.endedDown) {
		gameState->players[0].pos += dt * player0VelocityDown;
	}
	if(gameState->input[1].up.endedDown) {
		gameState->players[1].pos += dt * player1VelocityUp;
	}
	if(gameState->input[1].down.endedDown) {
		gameState->players[1].pos += dt * player1VelocityDown;
	}
}




#if !SOFTWARE
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

inline void quad(v2 vertices[], int n) {
	// n is assumed to be 4 or a multiple of 4.
	glBegin(GL_QUADS);

	for(int i=0; i < n; ++i) {
		glVertex2f(vertices[i].x, vertices[i].y);
	}

	glEnd();
}

inline void line(float x0, float y0, float x1, float y1) {
	glBegin(GL_LINES);

	glVertex2f(x0, y0);
	glVertex2f(x1, y1);

	glEnd();
}

void render(game_state *gameState, float offset) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	v2 ballOffset = (offset * gameState->ball.pos) + ((1.0f - offset) * gameState->ball.pos);
	v2 player0Offset = (offset * gameState->players[0].pos) + ((1.0f - offset) * gameState->players[0].pos);
	v2 player1Offset = (offset * gameState->players[1].pos) + ((1.0f - offset) * gameState->players[1].pos);

	makeRectFromCenterPoint(gameState->ball.vertices, ballOffset, gameState->ball.size);
	makeRectFromCenterPoint(gameState->players[0].vertices, player0Offset, gameState->players[0].size);
	makeRectFromCenterPoint(gameState->players[1].vertices, player1Offset, gameState->players[1].size);

	glColor3f(1.0f, 1.0f, 1.0f);

	line(Screen_Width / 2.0f, 0.0f, Screen_Width / 2.0f, Screen_Height);
	quad(gameState->players[0].vertices, 4);
	quad(gameState->players[1].vertices, 4);
	quad(gameState->ball.vertices, 4);

	glFlush();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	int64_t perfCountFrequency = perfCountFrequencyResult.QuadPart;

	timeBeginPeriod(1);

	WNDCLASSEX wc;
	wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(0, "WindowClass", "Pong", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	                      0, 0, Screen_Width, Screen_Height, NULL, NULL, hInstance, NULL);

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
	initGameState(gameState, Screen_Width, Screen_Height, V2(50, Player_Default_Y),
	              V2(Screen_Width - 50, Player_Default_Y), V2(Ball_Default_X, Ball_Default_Y),
	              V2(Player_Width, Player_Height), V2(Ball_Width, Ball_Height));

	LARGE_INTEGER previous = getWallClock();
	float accumulator = 0.0f;
	float targetSeconds = 1 / 480.0f;

	PFNWGLSWAPINTERVALEXTPROC proc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
#if VSYNC
	proc(1);
#else
	proc(0);
#endif
	while(gameState->programRunning) {
		processPendingMessages(gameState);

		LARGE_INTEGER current = getWallClock();
		int64_t microsecondsElapsed = getMicrosecondsElapsed(previous, current, perfCountFrequency);
		previous = current;

		// microsecondsElapsed is converted from microseconds to seconds and added to accumulator variable
		accumulator += (microsecondsElapsed / (1000.0f * 1000.0f));

		// while(accumulator >= targetSeconds) {
		// 	update(gameState, targetSeconds);
		// 	accumulator -= targetSeconds;
		// }

		float offset = accumulator / targetSeconds;
		update(gameState, targetSeconds);
		render(gameState, 0.0f);
		SwapBuffers(deviceContext);

		LARGE_INTEGER sleep = getWallClock();
		float remaining = getMicrosecondsElapsed(current, sleep, perfCountFrequency) / (1000.0f * 1000.0f);
		while(remaining < targetSeconds) {
			remaining = getMicrosecondsElapsed(current, getWallClock(), perfCountFrequency) / (1000.0f * 1000.0f);
		}

#if 1
		LARGE_INTEGER end = getWallClock();
		float msPerFrame = getMicrosecondsElapsed(previous, end, perfCountFrequency) / 1000.0f;

		char fpsBuffer[50];
		sprintf_s(fpsBuffer, "%.02fms/f\n", msPerFrame);

		SetWindowText(hWnd, fpsBuffer);
#endif
	}

	wglDeleteContext(renderContext);
	VirtualFree(gameMemory.storage, 0, MEM_RELEASE);
	return 0;
}
















#else
void resizeDIBSection(offscreen_buffer *buffer, int width, int height) {
	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;
	buffer->pitch = Align16(buffer->bytesPerPixel * buffer->width);

	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = (buffer->pitch * buffer->height);	
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void displayBufferInWindow(offscreen_buffer *buffer, HDC context) {
	StretchDIBits(context, 0, 0, buffer->width, buffer->height, 0, 0, buffer->width, buffer->height,
	              buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

inline void drawRectangle(offscreen_buffer *buffer, v2 vMin, v2 vMax, int color) {
	// vMin should be vertices[0] and vMax should be vertices[2]. At least for now.
	rectangle2i source = makeRectV2(vMin, vMax);
	rectangle2i dest = Rect(0, 0, buffer->width, buffer->height);
	rectangle2i clip = clipRect(source, dest);

	u8 *row = ((u8 *)buffer->memory + (clip.minX*buffer->bytesPerPixel) + 
	           (clip.minY*buffer->pitch));
	for(int y=clip.minY; y < clip.maxY; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		for(int x=clip.minX; x < clip.maxX; ++x) {
			*pixel++ = color;
		}
		row += buffer->pitch;
	}
}

inline void clearBuffer(offscreen_buffer *buffer) {
	u8 *row = (u8 *)buffer->memory;
	for(int y=0; y < buffer->height; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		for(int x=0; x < buffer->width; ++x) {
			*pixel++ = 0x00000000;
		}
		row += buffer->pitch;
	}
}

void render(game_state *gameState, offscreen_buffer *buffer, float offset) {
	clearBuffer(buffer);

	v2 ballOffset = (offset * gameState->ball.pos) + ((1.0f - offset) * gameState->ball.pos);
	v2 player0Offset = (offset * gameState->players[0].pos) + ((1.0f - offset) * gameState->players[0].pos);
	v2 player1Offset = (offset * gameState->players[1].pos) + ((1.0f - offset) * gameState->players[1].pos);

	makeRectFromCenterPoint(gameState->ball.vertices, ballOffset, gameState->ball.size);
	makeRectFromCenterPoint(gameState->players[0].vertices, player0Offset, gameState->players[0].size);
	makeRectFromCenterPoint(gameState->players[1].vertices, player1Offset, gameState->players[1].size);

	drawRectangle(buffer, gameState->ball.vertices[0], gameState->ball.vertices[2], 0xffffffff);
	drawRectangle(buffer, gameState->players[0].vertices[0], gameState->players[0].vertices[2], 0xffffffff);
	drawRectangle(buffer, gameState->players[1].vertices[0], gameState->players[1].vertices[2], 0xffffffff);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	uint64_t perfCountFrequency = perfCountFrequencyResult.QuadPart;


	WNDCLASSEX wc;
	wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);

	offscreen_buffer buffer = {};
	resizeDIBSection(&buffer, Screen_Width, Screen_Height);

	hWnd = CreateWindowEx(0, "WindowClass", "Pong", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	                      0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 
	                      0, hInstance, 0);

	HDC deviceContext = GetDC(hWnd);

	ShowWindow(hWnd, nCmdShow);

	game_memory gameMemory = {};
	gameMemory.storageSize = megabytes(1);
	gameMemory.storage = VirtualAlloc(0, (size_t)gameMemory.storageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	game_state *gameState = (game_state*)gameMemory.storage;

	initGameState(gameState, Screen_Width, Screen_Height, V2(50, Player_Default_Y), 
	              V2(Screen_Width - 50, Player_Default_Y), V2(Ball_Default_X, Ball_Default_Y), 
	              V2(Player_Width, Player_Height), V2(Ball_Width, Ball_Height));

	timeBeginPeriod(1);

	LARGE_INTEGER previous = getWallClock();
	float lag = 0.0f;
	float targetSeconds = 1 / 120.0f;

	while(gameState->programRunning) {
		processPendingMessages(gameState);

		LARGE_INTEGER current = getWallClock();
		uint64_t microsecondsElapsed = getMicrosecondsElapsed(previous, current, perfCountFrequency);
		previous = current;

		// microsecondsElapsed is converted from microseconds to seconds and added to lag variable
		lag += (microsecondsElapsed / (1000.0f * 1000.0f));

		while(lag >= targetSeconds) {
			update(gameState, targetSeconds);
			lag -= targetSeconds;
		}

		float offset = lag / targetSeconds;
		render(gameState, &buffer, offset);
		displayBufferInWindow(&buffer, deviceContext);

		// LARGE_INTEGER sleep = getWallClock();
		// float remaining = getMicrosecondsElapsed(previous, sleep, perfCountFrequency) / (1000.0f * 1000.0f);
		// while(remaining < targetSeconds) {
		// 	int64_t amount = (DWORD)((targetSeconds - remaining) * 1000.0f);
		// 	remaining = getMicrosecondsElapsed(previous, getWallClock(), perfCountFrequency) / (1000.0f * 1000.0f);
		// 	Sleep((DWORD)amount);
		// }

#if 0
		LARGE_INTEGER end = getWallClock();
		float msPerFrame = getMicrosecondsElapsed(previous, end, perfCountFrequency) / 1000.0f;

		char fpsBuffer[256];
		sprintf_s(fpsBuffer, "%.02fms/f\n", msPerFrame);

		OutputDebugString(fpsBuffer);
#endif
	}

	VirtualFree(gameMemory.storage, 0, MEM_RELEASE);
	VirtualFree(buffer.memory, 0, MEM_RELEASE);
	return 0;
}
#endif