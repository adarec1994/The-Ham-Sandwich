#include "stdafx.h"
#include "Window.h"
#include "GxContext.h"
#include "ScriptManager.h"

#include <DbgHelp.h>

std::wstring toUnicode(const std::string& ascii) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt;

	return cvt.from_bytes(ascii);
}

std::string toMultibyte(const std::wstring& unicode) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt;

	return cvt.to_bytes(unicode);
}

LONG WINAPI ExceptionFilter(LPEXCEPTION_POINTERS info) {
	std::wstringstream strm;
	strm << "WildstarStudio_" << std::chrono::high_resolution_clock::now().time_since_epoch().count() << ".dmp";
	auto file = CreateFile(strm.str().c_str(), FILE_WRITE_ACCESS | FILE_READ_ACCESS, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	MINIDUMP_EXCEPTION_INFORMATION exInfo;
	exInfo.ClientPointers = FALSE;
	exInfo.ExceptionPointers = info;
	exInfo.ThreadId = GetCurrentThreadId();

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithIndirectlyReferencedMemory, &exInfo, nullptr, nullptr);
	CloseHandle(file);

	std::wstringstream msgStrm;
	msgStrm << L"Wildstar studio has encountered a critical error! The essential information has been written into the file: " << strm.str() << std::endl << std::endl;
	msgStrm << L"You will now be redirected to the issue site where you can upload this file and describe the error you have encountered. Thank you.";
	MessageBox(nullptr, msgStrm.str().c_str(), L"Error!", MB_OK);

	ShellExecute(nullptr, L"open", L"https://bitbucket.org/mugadr_m/wildstar-studio/issues/new", nullptr, nullptr, SW_SHOW);
	ExitProcess(0);
}

BOOL WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT) {
	SetUnhandledExceptionFilter(ExceptionFilter);

	CoInitialize(nullptr);
	ULONG_PTR gdiToken = 0;
	Gdiplus::GdiplusStartupInput startInput;
	Gdiplus::GdiplusStartup(&gdiToken, &startInput, nullptr);

	WindowPtr wnd = std::make_shared<Window>();
	sGxCtx->init(wnd);

	sScriptMgr->initGlobalContext();

	wnd->syncRunLoop();

	Gdiplus::GdiplusShutdown(gdiToken);

	CoUninitialize();
}