#define UNICODE
#include <windows.h>

#define Assert(condition) { if(!(condition)) { DebugBreak(); } }

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch(message) {
		case WM_KEYDOWN: {
			if(wparam == VK_ESCAPE) {
				DestroyWindow(window);
			}
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		default: {
			return DefWindowProcW(window, message, wparam, lparam);
		}
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
	WNDCLASSEXW win_class = {};
	win_class.cbSize = sizeof(WNDCLASSEXW);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = &WindowProc;
	win_class.hInstance = instance;
	win_class.hIcon = LoadIconW(0, IDI_APPLICATION);
	win_class.hCursor = LoadCursorW(0, IDC_ARROW);
	win_class.lpszClassName = L"NeuroWindowClass";
	win_class.hIconSm = LoadIconW(0, IDI_APPLICATION);

	if(!RegisterClassExW(&win_class)) {
		Assert(false);
		return GetLastError();
	}

	RECT rect = {0, 0, 1024, 768};
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;

	HWND window = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
								  win_class.lpszClassName,
								  L"Neuro",
								  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  width, height, 0, 0, instance, 0);

	if(!window) {
		Assert(false);
		return GetLastError();
	}

	bool running = true;
	while(running) {
		MSG message = {};
		while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			if(message.message == WM_QUIT) {
				running = false;
			}

			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		Sleep(1);
	}

	return 0;
}