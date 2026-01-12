#pragma once

SHARED_FWD(File);

class HexEditor
{
	static ATOM gHexEditorClass;

	FilePtr mFile;
	WNDCLASSEX mWndClass;
	HWND mWindow;
	HWND mScrollBar;
	std::thread mMessageThread;
	bool mIsRunning;
	uint64 mBaseOffset = 0;
	uint32 mMaxScroll;
	bool mUseHex = false;
	uint64 mLastOffset = 0;
	uint64 mCurrentPosition = 0;
	HWND mPositionLabel;
	HFONT mGuiFont;

	BOOL offsetDlgProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

	std::function<BOOL (HWND, UINT, WPARAM, LPARAM)> mOffsetCallback;

	std::unique_ptr<Gdiplus::Font> mTextFont;
	std::unique_ptr<Gdiplus::Font> mDigitFont;
	std::unique_ptr<Gdiplus::Brush> mTextBrush;
	std::unique_ptr<Gdiplus::Pen> mLinePen;
	std::unique_ptr<Gdiplus::Pen> mLinePenThick;
	std::unique_ptr<Gdiplus::Brush> mGrayBrush;
	std::unique_ptr<Gdiplus::Bitmap> mMemBitmap;
	std::unique_ptr<Gdiplus::Graphics> mGraphics;
	std::unique_ptr<Gdiplus::Brush> mSelBrush;

	void onPaint();

	static LRESULT WINAPI WndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL WINAPI DlgProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT onMessage(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void drawImage();
	void showDialog(int32 id, std::function<BOOL(HWND, UINT, WPARAM, LPARAM)>* proc);

public:
	HexEditor(FilePtr file);

	~HexEditor() {
		close();

		if (mMessageThread.joinable()) {
			mMessageThread.join();
		}
	}

	void close() {
		mIsRunning = false;
	}

	void show();
};