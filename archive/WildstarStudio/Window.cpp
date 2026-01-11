#include "stdafx.h"
#include "Window.h"
#include "GxContext.h"
#include "UIManager.h"
#include "resource.h"

Window::Window() {
	memset(&mClass, 0, sizeof(mClass));

	mClass.hbrBackground = GetSysColorBrush(GetSysColor(COLOR_WINDOW));
	mClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	mClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1));
	mClass.hInstance = GetModuleHandle(nullptr);
	mClass.lpfnWndProc = &Window::WndProc;
	mClass.lpszClassName = L"GxWindowClass";
	mClass.style = CS_VREDRAW | CS_HREDRAW;

	if (RegisterClass(&mClass) == 0) {
		throw std::runtime_error("Unable to register window class");
	}

	mWindow = CreateWindow(L"GxWindowClass", L"Wildstar Studio", WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 1024, nullptr, nullptr, GetModuleHandle(nullptr), (LPVOID) this);

	RECT rc = { 0 };
	rc.bottom = 1024;
	rc.right = 1280;

	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, FALSE);

	SetWindowPos(mWindow, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
}

bool Window::parseMessages() {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

void Window::syncRunLoop() {
	ShowWindow(mWindow, SW_SHOWMAXIMIZED);

	while (parseMessages()) {
		sGxCtx->onFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

uint32 Window::getWidth() const {
	RECT rc;
	GetClientRect(mWindow, &rc);
	return rc.right - rc.left;
}

uint32 Window::getHeight() const {
	RECT rc;
	GetClientRect(mWindow, &rc);
	return rc.bottom - rc.top;
}

LRESULT Window::onMessage(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LRESULT resOverride = 0;
	bool globalHandled = false;
	for (auto& handler : mMessageHandlers) {
		bool handled = false;
		LRESULT res;
		handler(uMsg, wParam, lParam, handled, res);
		if (handled) {
			globalHandled = true;
			resOverride = res;
		}
	}

	if (uMsg == WM_CLOSE) {
		sUIMgr->shutdown();
		PostQuitMessage(0);
		return 0;
	}

	if (globalHandled) {
		return resOverride;
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}

LRESULT WINAPI Window::WndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window* wnd = (Window*) GetProp(hWindow, L"PROP_WND_INSTANCE");
	if (wnd != nullptr) {
		return wnd->onMessage(hWindow, uMsg, wParam, lParam);
	}

	if (uMsg == WM_CREATE) {
		wnd = (Window*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
		SetProp(hWindow, L"PROP_WND_INSTANCE", (HANDLE) wnd);
		return wnd->onMessage(hWindow, uMsg, wParam, lParam);
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}