#include "precompiled.h"

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

struct offscreen_buffer {
	void* memory;
	s32 height;
	s32 width;
	s32 bitsPerPixel;
	s32 pitch;
};

struct game_memory {
	void* storage;
	u32 storageSize;

	bool isInitialized;
};

struct player {
	float paddleX_, paddleY;
};

struct ball {
	float ballX, ballY;
};

struct game_state {
	player players[2];
	float scores[2];
	ball theBall;

	v2* staticBoardVertices;
};

void handleKeyDown(int vkCode) {
	if(vkCode == VK_ESCAPE)
		PostQuitMessage(0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_KEYDOWN: {
			handleKeyDown(wParam);
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

	glEnable(GL_TEXTURE_2D);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		OutputDebugString("Error initializing OpenGL!\n");
		return false;
	}
	return true;
}

bool loadTextureFromPixels(void* pixels) {
	glGenTextures(1, &tex->textureId);

	glBindTexture(GL_TEXTURE_2D, tex->textureId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->textureWidth, tex->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, NULL);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		OutputDebugString("Error loading texture\n");
		return false;
	}

	return true;
}

bool loadMedia(texture* tex, int width, int height) {
	tex->pixels = new unsigned int[width * height];
	tex->textureWidth = width;
	tex->textureHeight = height;

	for(int i=0; i < (width * height); i++) {
		int check = i / width & 16 ^ i % height & 16;
		if(check)
			tex->pixels[i] = (0xff << 24) | (0xff << 16) | (0xff << 8) | (0xff << 0);
		else
			tex->pixels[i] = (0xff << 24) | (0x00 << 16) | (0x00 << 8) | (0xff << 0);
	}

	bool load = loadTextureFromPixels(tex);
	if(!load) {
		OutputDebugString("Can't load checkerboard texture\n");
		return false;
	}
	return true;
}

void textureRender(float x, float y) {
	if(tex.textureId) {
		glLoadIdentity();

		glTranslatef(x, y, 0.0f);

		glBindTexture(GL_TEXTURE_2D, tex.textureId);

		glBegin(GL_QUADS);

		glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f((float)tex.textureWidth, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f((float)tex.textureWidth, (float)tex.textureHeight);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)tex.textureHeight);

		glEnd();
	}
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	float x = (SCREEN_WIDTH - 2) / 2.0f;
	float y = (SCREEN_HEIGHT - 2) / 2.0f;

	textureRender(x, y);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;
	WNDCLASSEX wc;

	wc ={ 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL,
												"WindowClass",
												"OpenGL Template",
												WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
												0, 0,
												SCREEN_WIDTH, SCREEN_HEIGHT,
												NULL,
												NULL,
												hInstance,
												NULL);

	PIXELFORMATDESCRIPTOR pfd ={
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

	PFNWGLSWAPINTERVALEXTPROC proc = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if(proc)
		proc(-1);

	ShowWindow(hWnd, nCmdShow);

	game_memory gameMemory = {};
	gameMemory.storageSize = kilobytes(1);

	if(!initGL() || !loadMedia(&mainTexture, 256, 256))
		PostQuitMessage(0);

	MSG msg;

	int frames = 0;
	unsigned long timer = GetTickCount();
	char fpsBuffer[20];

	bool running = true;

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

		render(mainTexture);
		SwapBuffers(deviceContext);

		frames++;

		if(GetTickCount() - timer > 1000) {
			timer += 1000;
			wsprintf(fpsBuffer, "FPS: %d\n", frames);
			OutputDebugString(fpsBuffer);
			frames = 0;
		}
	}

	wglDeleteContext(renderContext);
	return msg.wParam;
}