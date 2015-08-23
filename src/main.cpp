#include "precompiled.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

struct texture {
	unsigned int textureId;
	int textureWidth;
	int textureHeight;
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

void freeTexture(texture *tex) {
	if(tex->textureId) {
		glDeleteTextures(1, &tex->textureId);
		tex->textureId = 0;
	}

	tex->textureHeight = 0;
	tex->textureWidth = 0;
}

bool loadTextureFromPixels(unsigned int* pixels, int width, int height, texture* tex) {
	freeTexture(tex);

	tex->textureWidth = width;
	tex->textureHeight = height;

	glGenTextures(1, &tex->textureId);

	glBindTexture(GL_TEXTURE_2D, tex->textureId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

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

bool loadMedia(texture *tex) {
	unsigned int checkerBoard[128 * 128];

	for(int i=0; i < (128* 128); i++) {
		int check = i / 128 & 16 ^ i % 128 & 16;
		if(check)
			checkerBoard[i] = (0xff << 24) | (0xff << 16) | (0xff << 8) | (0xff << 0);
		else
			checkerBoard[i] = (0xff << 24) | (0x00 << 16) | (0x00 << 8) | (0xff << 0);
	}

	bool load = loadTextureFromPixels(checkerBoard, 128, 128, tex);
	if(!load) {
		OutputDebugString("Can't load checkerboard texture\n");
		return false;
	}
	return true;
}

void textureRender(texture tex, float x, float y) {
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

void render(texture tex) {
	glClear(GL_COLOR_BUFFER_BIT);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	float x = (SCREEN_WIDTH - tex.textureWidth) / 2.0f;
	float y = (SCREEN_HEIGHT - tex.textureHeight) / 2.0f;

	textureRender(tex, x, y);
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

#if CHAT_
	askToHostOrConnect();
#endif

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

	texture mainTexture;
	if(!initGL() || !loadMedia(&mainTexture))
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