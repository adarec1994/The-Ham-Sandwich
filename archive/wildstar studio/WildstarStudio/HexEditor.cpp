#include "stdafx.h"
#include "HexEditor.h"
#include "Files.h"
#include "resource.h"

ATOM HexEditor::gHexEditorClass = -1;

LRESULT WINAPI HexEditor::WndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HexEditor* he = (HexEditor*) GetProp(hWindow, L"PROP_WND_INSTANCE");
	if (he != nullptr) {
		return he->onMessage(hWindow, uMsg, wParam, lParam);
	}

	if (uMsg == WM_CREATE) {
		auto lpc = (LPCREATESTRUCT) lParam;
		SetProp(hWindow, L"PROP_WND_INSTANCE", lpc->lpCreateParams);
		HexEditor* he = (HexEditor*) GetProp(hWindow, L"PROP_WND_INSTANCE");
		return he->onMessage(hWindow, uMsg, wParam, lParam);
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}

HexEditor::HexEditor(FilePtr file) {
	mWindow = nullptr;
	mIsRunning = false;

	mOffsetCallback = std::bind(&HexEditor::offsetDlgProc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_3);

	mFile = file;
	mWndClass.hInstance = GetModuleHandle(nullptr);

	if (gHexEditorClass == (ATOM) -1) {
		memset(&mWndClass, 0, sizeof(WNDCLASSEX));
		mWndClass.cbSize = sizeof(WNDCLASSEX);
		mWndClass.hbrBackground = GetSysColorBrush(GetSysColor(COLOR_WINDOW));
		mWndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		mWndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		mWndClass.hIconSm = mWndClass.hIcon;
		mWndClass.hInstance = GetModuleHandle(nullptr);
		mWndClass.lpfnWndProc = &HexEditor::WndProc;
		mWndClass.lpszClassName = L"HexEditorClass";
		mWndClass.style = CS_HREDRAW | CS_VREDRAW;

		gHexEditorClass = RegisterClassEx(&mWndClass);
	}

	mTextFont = std::unique_ptr<Gdiplus::Font>(new Gdiplus::Font(L"Consolas", 10));
	mDigitFont = std::unique_ptr<Gdiplus::Font>(new Gdiplus::Font(L"Consolas", 8));
	mTextBrush = std::unique_ptr<Gdiplus::Brush>(new Gdiplus::SolidBrush(Gdiplus::Color::Black));
	mGrayBrush = std::unique_ptr<Gdiplus::Brush>(new Gdiplus::SolidBrush(Gdiplus::Color::Gray));
	mLinePen = std::unique_ptr<Gdiplus::Pen>(new Gdiplus::Pen(mTextBrush.get(), 1.5f));
	mLinePenThick = std::unique_ptr<Gdiplus::Pen>(new Gdiplus::Pen(mTextBrush.get(), 2.5f));
	mMemBitmap = std::unique_ptr<Gdiplus::Bitmap>(new Gdiplus::Bitmap(1093, 600));
	uint32 color = GetSysColor(COLOR_HIGHLIGHT) | 0xFF000000;
	color = 0xFF000000 | ((color & 0x00FF0000) >> 16) | (color & 0x0000FF00) | ((color & 0x000000FF) << 16);
	mSelBrush = std::unique_ptr<Gdiplus::Brush>(new Gdiplus::SolidBrush(Gdiplus::Color(color)));
	mGraphics = std::unique_ptr<Gdiplus::Graphics>(Gdiplus::Graphics::FromImage(mMemBitmap.get()));

	drawImage();
}

void HexEditor::show() {
	if (mIsRunning == true) {
		return;
	}

	mIsRunning = true;
	mMessageThread = std::thread([this]() {
		MSG msg;
		mWindow = CreateWindow(L"HexEditorClass", mFile->getFileName().c_str(), WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 1300, 600, nullptr, nullptr, mWndClass.hInstance, this);
		RECT rc;
		rc.top = rc.left = 0;
		rc.bottom = 630;
		rc.right = 1093 + GetSystemMetrics(SM_CXVSCROLL);
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, FALSE);
		SetWindowPos(mWindow, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

		mScrollBar = CreateWindow(L"Scrollbar", L"", WS_CHILD | WS_VISIBLE | SBS_VERT , 1093, 0, GetSystemMetrics(SM_CXVSCROLL), 600, mWindow, nullptr, mWndClass.hInstance, nullptr);
		mMaxScroll = (uint32) ((mFile->getFileSize() / 16) + (mFile->getFileSize() % 16 ? 1 : 0));
		SetScrollRange(mScrollBar, SB_CTL, 0, mMaxScroll, TRUE);

		mPositionLabel = CreateWindow(L"STATIC", L"Current Position: 00000000", WS_CHILD | WS_VISIBLE | SS_LEFT, 5, 605, 1000, 25, mWindow, nullptr, mWndClass.hInstance, nullptr);
		NONCLIENTMETRICS ncm = { 0 };
		ncm.cbSize = sizeof(ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 1, &ncm, SPIF_SENDCHANGE);
		mGuiFont = CreateFontIndirect(&ncm.lfMessageFont);
		SendMessage(mPositionLabel, WM_SETFONT, (WPARAM) mGuiFont, TRUE);

		SCROLLINFO si = { 0 };
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE;
		si.nPage = 28;
		SetScrollInfo(mScrollBar, SB_CTL, &si, TRUE);

		ShowWindow(mWindow, SW_SHOW);

		while (mIsRunning) {
			while (PeekMessage(&msg, mWindow, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					mIsRunning = false;
					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(33));
		}

		DeleteFont(mGuiFont);
	});
}

void HexEditor::drawImage() {
	std::wstringstream strm;
	strm << L"Current Position: " << std::hex << std::uppercase << std::setfill(L'0') << std::setw(8) << mCurrentPosition;
	SetWindowText(mPositionLabel, strm.str().c_str());

	using namespace Gdiplus;

	static const wchar_t digits[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

	Graphics* g = mGraphics.get();
	g->Clear(Color::White);

	g->SetTextRenderingHint(TextRenderingHintAntiAlias);

	g->FillRectangle(mGrayBrush.get(), RectF(0.0f, 0.0f, 1090.0f, 20.0f));
	g->FillRectangle(mGrayBrush.get(), RectF(0.0f, 0.0f, 130.0f, 600.0f));

	for (uint32 i = 0; i < 16; ++i) {
		g->DrawLine(mLinePen.get(), PointF((i + 1) * 30.0f + 100, 0.0f), PointF((i + 1) * 30.0f + 100, 600.0f));
		g->DrawLine(mLinePen.get(), PointF((18 + i) * 30.0f + 100.0f, 0.0f), PointF((18 + i) * 30.0f + 100.0f, 600.0f));
	}

	g->DrawLine(mLinePenThick.get(), PointF(17 * 30.0f + 100.0f, 0.0f), PointF(17 * 30.0f + 100.0f, 600.0f));

	for (uint32 i = 0; i < 29; ++i) {
		g->DrawLine(mLinePen.get(), PointF(0, (i + 1) * 20.0f), PointF(1090.0f, (i + 1) * 20.0f));
	}

	for (uint32 i = 0; i < 16; ++i) {
		g->DrawString(digits + i, 1, mTextFont.get(), PointF(139.0f + i * 30, 1), mTextBrush.get());
		g->DrawString(digits + i, 1, mTextFont.get(), PointF((17 + i) * 30.0f + 109.0f, 0.0f), mTextBrush.get());
	}

	for (uint32 i = 0; i < 29; ++i) {
		uint64 ofs = mBaseOffset + i * 16;
		std::wstringstream strm;
		strm << std::setw(8) << std::setfill(L'0') << std::hex << ofs;
		g->DrawString(strm.str().c_str(), strm.str().length(), mTextFont.get(), PointF(10, 21.0f + i * 20.0f), mTextBrush.get());
		strm = std::wstringstream();
		for (uint32 j = 0; j < 16; ++j) {
			uint64 curPos = ofs + j;
			if (curPos == mCurrentPosition) {
				g->FillRectangle(mSelBrush.get(), RectF(131.0f + j * 30.0f, 21.0f + i * 20.0f, 29.0f, 19.0f));
				g->FillRectangle(mSelBrush.get(), RectF(611.0f + j * 30.0f, 21.0f + i * 20.0f, 29.0f, 19.0f));
			}

			g->DrawString(digits + (mFile->getContent()[(uint32)ofs + j] >> 4), 1, mDigitFont.get(), PointF(137.0f + j * 30, 23.0f + i * 20.0f), mTextBrush.get());
			g->DrawString(digits + (mFile->getContent()[(uint32)ofs + j] & 0xF), 1, mDigitFont.get(), PointF(145.0f + j * 30, 23.0f + i * 20.0f), mTextBrush.get());

			wchar_t c = (wchar_t) mFile->getContent()[(uint32) ofs + j];
			if (isprint(c) == false) {
				c = L'.';
			}

			g->DrawString(&c, 1, mDigitFont.get(), PointF(621.0f + j * 30.0f, 23.0f + i * 20.0f), mTextBrush.get());
		}
	}

	g->Flush();
}

void HexEditor::onPaint() {
	PAINTSTRUCT ps = { 0 };
	HDC hDc = BeginPaint(mWindow, &ps);

	Gdiplus::Graphics* g = Gdiplus::Graphics::FromHDC(hDc);
	g->DrawImage(mMemBitmap.get(), 0, 0);
	delete g;

	EndPaint(mWindow, &ps);
}

void HexEditor::showDialog(int32 id, std::function<BOOL(HWND, UINT, WPARAM, LPARAM)>* proc) {
	auto dlg = DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(id), mWindow, &HexEditor::DlgProc, (LPARAM) proc);
}

BOOL HexEditor::offsetDlgProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			std::wstringstream strm;
			strm << (mUseHex ? std::hex : std::dec) << mLastOffset;

			Button_SetCheck(GetDlgItem(hWindow, IDC_RADIO1), !mUseHex);
			Button_SetCheck(GetDlgItem(hWindow, IDC_RADIO2), mUseHex);
			SetDlgItemText(hWindow, IDC_EDIT1, strm.str().c_str());
		}
		return TRUE;
		
	case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(lParam) == IDC_RADIO1) {
					mUseHex = false;
					wchar_t text[50] = { L'\0' };
					GetDlgItemText(hWindow, IDC_EDIT1, text, 50);
					std::wstringstream strm;
					strm << text;
					uint64 val;
					if (strm >> std::hex >> val) {
						strm = std::wstringstream();
						strm << std::dec << val;
					} else {
						strm << 0;
					}

					SetDlgItemText(hWindow, IDC_EDIT1, strm.str().c_str());
				} else if (LOWORD(lParam) == IDC_RADIO2) {
					mUseHex = true;
					wchar_t text[50] = { L'\0' };
					GetDlgItemText(hWindow, IDC_EDIT1, text, 50);
					std::wstringstream strm;
					strm << text;
					uint64 val;
					if (strm >> val) {
						strm = std::wstringstream();
						strm << std::hex << val;
					} else {
						strm << 0;
					}

					SetDlgItemText(hWindow, IDC_EDIT1, strm.str().c_str());
				} else if (LOWORD(lParam) == IDC_GO_BUTTON) {
					wchar_t text[50] = { L'\0' };
					GetDlgItemText(hWindow, IDC_EDIT1, text, 50);
					uint64 ofs = 0;
					std::wstringstream strm;
					strm << text;
					if (strm >> (mUseHex ? std::hex : std::dec) >> ofs) {
						mBaseOffset = ofs & ~(uint64) 15;
						mCurrentPosition = ofs;

						SetScrollPos(mScrollBar, SB_CTL, (uint32) (mBaseOffset / 16), TRUE);

						drawImage();

						RECT rc;
						rc.top = rc.left = 0;
						rc.right = 1090;
						rc.bottom = 600;
						RedrawWindow(mWindow, &rc, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
					}

					SendMessage(hWindow, WM_CLOSE, 0, 0);
				}
			}
		}
		return FALSE;

	case WM_CLOSE:
		EndDialog(hWindow, 0);
		return TRUE;

	default:
		return FALSE;
	}
}

BOOL WINAPI HexEditor::DlgProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	auto fun = (std::function<BOOL(HWND, UINT, WPARAM, LPARAM)>*) GetProp(hWindow, L"PROP_DLG_CALLBACK");
	if (fun != nullptr) {
		return (*fun)(hWindow, uMsg, wParam, lParam);
	}

	if (uMsg == WM_INITDIALOG) {
		fun = (std::function<BOOL(HWND, UINT, WPARAM, LPARAM)>*) lParam;
		SetProp(hWindow, L"PROP_DLG_CALLBACK", (HANDLE) fun);
		return (*fun)(hWindow, uMsg, wParam, lParam);
	}

	return FALSE;
}

LRESULT HexEditor::onMessage(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_CLOSE) {
		mIsRunning = false;
		return 0;
	}

	switch (uMsg) {
	case WM_PAINT:
		onPaint();
		return TRUE;

	case WM_MOUSEWHEEL:
		{
			int32 delta = GET_WHEEL_DELTA_WPARAM(wParam) / -120;
			if (delta < 0) {
				uint64 sub = -delta * 16;
				sub = std::min(sub, mBaseOffset);
				mCurrentPosition -= sub;
				if (mCurrentPosition >= mMaxScroll) {
					mBaseOffset = mMaxScroll - 1;
					mCurrentPosition = mMaxScroll - 1;
				}

				if (mCurrentPosition < mBaseOffset) {
					mBaseOffset = mCurrentPosition & ~15;
				}

				if (mCurrentPosition > mBaseOffset + 28 * 16) {
					mBaseOffset = ((mCurrentPosition & ~15) - 28 * 16);
				}
			} else {
				mCurrentPosition += (uint32) delta * 16;
				if (mCurrentPosition >= mMaxScroll) {
					mBaseOffset = mMaxScroll - 1;
					mCurrentPosition = mMaxScroll - 1;
				}

				if (mCurrentPosition < mBaseOffset) {
					mBaseOffset = mCurrentPosition & ~15;
				}

				if (mCurrentPosition > mBaseOffset + 28 * 16) {
					mBaseOffset = ((mCurrentPosition & ~15) - 28 * 16);
				}
			}

			SetScrollPos(mScrollBar, SB_CTL, (uint32)(mBaseOffset / 16), TRUE);

			drawImage();

			RECT rc;
			rc.top = rc.left = 0;
			rc.right = 1090;
			rc.bottom = 600;
			RedrawWindow(mWindow, &rc, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
		}
		break;

	case WM_CHAR:
		{
			if (wParam == 7) {
				showDialog(IDD_OFFSET_DIALOG, &mOffsetCallback);
			}
		}
		break;

	case WM_KEYDOWN:
		{
			if (wParam == VK_DOWN) {
				mCurrentPosition += 16;
			} else if (wParam == VK_UP) {
				if (mCurrentPosition >= 16) {
					mCurrentPosition -= 16;
				}
			} else if (wParam == VK_LEFT) {
				if (mCurrentPosition >= 1) {
					mCurrentPosition -= 1;
				}
			} else if (wParam == VK_RIGHT) {
				mCurrentPosition += 1;
			}

			if (mCurrentPosition < mBaseOffset) {
				mBaseOffset = mCurrentPosition & ~15;
			}

			if (mCurrentPosition > mBaseOffset + 28 * 16) {
				mBaseOffset = ((mCurrentPosition & ~15) - 28 * 16);
			}

			SetScrollPos(mScrollBar, SB_CTL, (uint32) (mBaseOffset / 16), TRUE);

			drawImage();

			RECT rc;
			rc.top = rc.left = 0;
			rc.right = 1090;
			rc.bottom = 600;
			RedrawWindow(mWindow, &rc, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
		}
		break;

	case WM_VSCROLL:
		{
			uint32 curPos = GetScrollPos(mScrollBar, SB_CTL);
			SCROLLINFO sibf = { 0 };
			sibf.cbSize = sizeof(SCROLLINFO);
			sibf.fMask = SIF_ALL;

			GetScrollInfo(mScrollBar, SB_CTL, &sibf);

			switch (LOWORD(wParam))
			{
			case SB_TOP:
				curPos = 0;
				break;

			case SB_LINEUP:
				if (curPos > 0)
					--curPos;
				break;

			case SB_THUMBPOSITION:
				break;

			case SB_THUMBTRACK:
				curPos = sibf.nTrackPos;
				break;
				
			case SB_PAGEDOWN:
				curPos += sibf.nPage;
				if (curPos >= mMaxScroll) {
					curPos = mMaxScroll - 1;
				}
				break;

			case SB_PAGEUP:
				curPos -= std::min(curPos, sibf.nPage);
				break;

			case SB_LINEDOWN:
				if (curPos < mMaxScroll)
					++curPos;
				break;

			case SB_BOTTOM:
				curPos = mMaxScroll;
				break;

			case SB_ENDSCROLL:
				break;
			}

			mBaseOffset = 16 * curPos;

			if (mCurrentPosition < mBaseOffset) {
				mCurrentPosition = mBaseOffset + mCurrentPosition % 16;
			}

			if (mCurrentPosition > mBaseOffset + 29 * 16) {
				mCurrentPosition = (mBaseOffset + 28 * 16) + mCurrentPosition % 16;
			}

			drawImage();

			SetScrollPos(mScrollBar, SB_CTL, curPos, TRUE);

			RECT rc;
			rc.top = rc.left = 0;
			rc.right = 1090;
			rc.bottom = 600;
			RedrawWindow(mWindow, &rc, nullptr, RDW_INTERNALPAINT | RDW_INVALIDATE);
		}
		break;

	case WM_CTLCOLORSTATIC:
		if ((HWND) lParam == mPositionLabel) {
			SetBkMode((HDC) wParam, OPAQUE);
			return (LRESULT) CreateSolidBrush(RGB(255, 255, 255));
		}
		break;
	}

	return DefWindowProc(hWindow, uMsg, wParam, lParam);
}