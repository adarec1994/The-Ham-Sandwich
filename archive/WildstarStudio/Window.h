#pragma once

class Window
{
	WNDCLASS mClass;
	HWND mWindow;
	std::list<std::function<void (UINT, WPARAM, LPARAM, bool&, LRESULT&)>> mMessageHandlers;

	static LRESULT WINAPI WndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT onMessage(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	Window();

	bool parseMessages();

	void syncRunLoop();

	void addMessageHandler(std::function<void (UINT, WPARAM, LPARAM, bool&, LRESULT&)> handler) { mMessageHandlers.push_back(handler); }

	HWND getHandle() const { return mWindow; }

	uint32 getWidth() const;
	uint32 getHeight() const;
};

SHARED_TYPE(Window);