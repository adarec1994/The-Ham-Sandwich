#include "stdafx.h"
#include "UIManager.h"
#include "IOManager.h"
#include "Texture.h"
#include "DataTable.h"
#include "ScriptManager.h"
#include "File.h"
#include "AreaFile.h"
#include "Shader.h"
#include <initguid.h>
#include <dsound.h>

UIManagerPtr UIManager::gInstance = nullptr;

using namespace Awesomium;

std::wstring escapeJsonString(const std::wstring& input) {
	std::wostringstream ss;
	for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
		if ((*iter) == L'\n') {
			ss << L"\\n";
			continue;
		}

		if (std::iswalnum(*iter) == false && std::iswprint(*iter) == false) {
			continue;
		}

		switch (*iter) {
		case L'\\': ss << L"\\\\"; break;
		case L'"': ss << L"\\\""; break;
		case L'/': ss << L"\\/"; break;
		case L'\b': ss << L"\\b"; break;
		case L'\f': ss << L"\\f"; break;
		case L'\n': ss << L"\\n"; break;
		case L'\r': ss << L"\\r"; break;
		case L'\t': ss << L"\\t"; break;
		default: ss << *iter; break;
		}
	}
	return ss.str();
}

UIManager::UIManager() {
	mView = nullptr;
	mIsAsyncLoadComplete = false;
	mTextureAsBmp = false;
	mTblAsCsv = false;
	mTblAsSql = false;
}

UIManager::~UIManager() {
	if (mView != nullptr) {
		mView->Destroy();
	}

	WebCore::Shutdown();
}

void UIManager::OnRequest(int request_id, const WebString& path) {
	if (path.length() == 0) {
		SendResponse(request_id, 0, nullptr, WSLit(""));
		return;
	}

	std::wstringstream strm;
	std::wstring str((const wchar_t*) path.data(), path.length());
	std::replace(str.begin(), str.end(), L'/', L'\\');

	if (str.find(L".filesys") != std::wstring::npos) {
		return onFileSystemRequest(request_id, path);
	}

	FileLoadRequest req;
	req.fileName = str;
	req.callback = 
		[this, request_id](uint32 size, void* data, WebString mime) {
			SendResponse(request_id, size, (unsigned char*) data, mime);
		};

	mLoadThread.pushRequest(req);
}

void UIManager::shutdown() {
	mLoadThread.shutdown();
}

void UIManager::OnAddConsoleMessage(Awesomium::WebView* caller, const Awesomium::WebString& message, int line_number, const Awesomium::WebString& source) {
	std::wstring msg((const wchar_t*)message.data(), message.length());
	std::wstring file((const wchar_t*) source.data(), source.length());

	MessageBoxW(nullptr, msg.c_str(), file.c_str(), MB_OK);
}

void UIManager::init(uint32 width, uint32 height, WindowPtr wnd) {
	mWindow = wnd;

	std::wstringstream strm;
	wchar_t curDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curDir);
	strm << curDir << L"\\bin";

	std::wstringstream childPath;
	childPath << curDir << L"\\bin\\" << L"awesomium_process.exe";

	WebConfig cfg;
	cfg.child_process_path = WebString((const wchar16*) (childPath.str().c_str()));
	cfg.package_path = WebString((const wchar16*) (strm.str().c_str()));
	cfg.plugin_path = cfg.package_path;
	cfg.log_level = LogLevel::kLogLevel_Verbose;
	cfg.log_path = WSLit("./debug.txt");
//#ifdef _DEBUG
	cfg.remote_debugging_host = WSLit("127.0.0.1");
	cfg.remote_debugging_port = 55378;
//#endif

	mCore = WebCore::Initialize(cfg);
	WebSession* session = mCore->CreateWebSession(WSLit(""), WebPreferences());

	session->AddDataSource(WSLit("local"), this);

	mView = mCore->CreateWebView(width, height, session);

	initJavascript();

	mView->SetTransparent(true);
	mView->set_view_listener(this);
	mView->set_js_method_handler(this);
	mView->LoadURL(WebURL(WSLit("asset://local/index.html")));
	mView->Focus();

	wnd->addMessageHandler(
		[this](UINT uMsg, WPARAM wParam, LPARAM lParam, bool& handled, LRESULT& res) {
			handled = false;

			switch (uMsg) {
			case WM_LBUTTONDOWN: 
				mView->InjectMouseDown(MouseButton::kMouseButton_Left);
				break;

			case WM_RBUTTONDOWN:
				mView->InjectMouseDown(MouseButton::kMouseButton_Right);
				break;

			case WM_MBUTTONDOWN:
				mView->InjectMouseDown(MouseButton::kMouseButton_Middle);
				break;

			case WM_LBUTTONUP:
				mView->InjectMouseUp(MouseButton::kMouseButton_Left);
				break;

			case WM_RBUTTONUP:
				mView->InjectMouseUp(MouseButton::kMouseButton_Right);
				break;

			case WM_MBUTTONUP:
				mView->InjectMouseUp(MouseButton::kMouseButton_Middle);
				break;

			case WM_MOUSEMOVE:
				//OnChangeCursor(mView, mCursor);
				mView->InjectMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				break;

			case WM_MOUSEWHEEL:
				mView->InjectMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam), 0);
				break;

			case WM_SETCURSOR:
				OnChangeCursor(mView, mCursor);
				handled = true;
				res = TRUE;
				break;

			case WM_SIZE:
				{
					if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
						mWidth = GET_X_LPARAM(lParam);
						mHeight = GET_Y_LPARAM(lParam);
						mView->Resize(mWidth, mHeight);
						glViewport(0, 0, mWidth, mHeight);
						mModelRender.resize(mWidth, mHeight);
						mAreaRender.resize(mWidth, mHeight);
					}
				}
				break;

			case WM_CHAR:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYUP:
			case WM_KEYDOWN:
				{
					WebKeyboardEvent ev(uMsg, wParam, lParam);
					mView->InjectKeyboardEvent(ev);
					break;
				}
			}
		}
	);

	mWidth = width;
	mHeight = height;

	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	uint32 colors[] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0x00000000 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors);
	glBindTexture(GL_TEXTURE_2D, 0);

	initRegistryValues();
	initWwise();

	mModelRender.initGraphics(width - 300, height - 150);
	mAreaRender.initGraphics(width - 300, height - 150);
}

void UIManager::onFrame() {
	{
		std::lock_guard<std::mutex> l(mSyncFrameLock);
		for (auto frame : mSyncFrames) {
			frame();
		}

		mSyncFrames.clear();
	}

	asyncUpdate();

	mCore->Update();

	BitmapSurface* bms = (BitmapSurface*) mView->surface();
	if (bms == nullptr) {
		return;
	}

	glEnable(GL_TEXTURE_2D);

	if (mActiveTexture != nullptr) {
		glBindTexture(GL_TEXTURE_2D, mActiveTexture->getId());

		uint32 h = mActiveTexture->getHeader().height;
		uint32 w = mActiveTexture->getHeader().width;

		glBegin(GL_TRIANGLE_FAN);

		static DWORD timeStart = 0;
		static float timeDelta = 0.0f;
		DWORD currentTime = GetTickCount();

		if (timeStart == 0)
			timeStart = currentTime;
		timeDelta = (currentTime - timeStart) / 1000.0f;
		timeStart = currentTime;


		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000 && GetForegroundWindow() == mWindow->getHandle())
		{
			static POINT lastMousePos;
			static POINT currentMousePos;
			GetCursorPos(&currentMousePos);
			POINT relativeMousePos = currentMousePos;
			ScreenToClient(mWindow->getHandle(), &relativeMousePos);
			if ((relativeMousePos.x < 300 || relativeMousePos.y < 100) && mLMouseDown == false) {
				lastMousePos = currentMousePos;
			} else {
				if (!mLMouseDown)
					GetCursorPos(&lastMousePos);

				auto x = lastMousePos.x - currentMousePos.x;
				auto y = lastMousePos.y - currentMousePos.y;

				mImageX += x * timeDelta * mMouseSensitivity * 0.4f;
				mImageY += y * timeDelta * mMouseSensitivity * 0.4f;

				lastMousePos = currentMousePos;
				mLMouseDown = true;
			}
		}  else
			mLMouseDown = false;

		float sx = (650.0f / mWindow->getWidth()) * 2 - 1 + mImageX;
		float sy = (200.0f / mWindow->getHeight()) * 2 + mImageY;
		float ex = ((650.0f + w) / mWindow->getWidth()) * 2 - 1 + mImageX;
		float ey = ((200.0f + h) / mWindow->getHeight()) * 2 + mImageY;
		sy = 1 - sy;
		ey = 1 - ey;

		glTexCoord2f(0, 0);
		glVertex2f(sx, sy);

		glTexCoord2f(1, 0);
		glVertex2f(ex, sy);

		glTexCoord2f(1, 1);
		glVertex2f(ex, ey);

		glTexCoord2f(0, 1);
		glVertex2f(sx, ey);

		glEnd();
	} else if (mActiveModel != nullptr || mActiveI3 != nullptr) {
		if (mActiveI3 != nullptr) {
			mModelRender.renderModel(mActiveI3);
		} else {
			mModelRender.renderModel(mActiveModel);
		}
		glViewport(0, 0, mWindow->getWidth(), mWindow->getHeight());

		float sx = (300.0f / mWindow->getWidth()) * 2 - 1;
		float sy = (100.0f / mWindow->getHeight()) * 2;
		sy = 1 - sy;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mModelRender.getTexture());
		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0, 0);
		glVertex2f(sx, sy);

		glTexCoord2f(1, 0);
		glVertex2f(1, sy);

		glTexCoord2f(1, 1);
		glVertex2f(1, -1);

		glTexCoord2f(0, 1);
		glVertex2f(sx, -1);
		
		glEnd();
	} else if (mActiveArea != nullptr) {
		mAreaRender.renderArea(mActiveArea);
		glViewport(0, 0, mWindow->getWidth(), mWindow->getHeight());

		float sx = (300.0f / mWindow->getWidth()) * 2 - 1;
		float sy = (100.0f / mWindow->getHeight()) * 2;
		sy = 1 - sy;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mAreaRender.getTexture());
		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0, 0);
		glVertex2f(sx, sy);

		glTexCoord2f(1, 0);
		glVertex2f(1, sy);

		glTexCoord2f(1, 1);
		glVertex2f(1, -1);

		glTexCoord2f(0, 1);
		glVertex2f(sx, -1);

		glEnd();
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	if (bms->is_dirty()) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bms->width(), bms->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, bms->buffer());
		bms->set_is_dirty(false);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1);

	glTexCoord2f(1, 0);
	glVertex2f(1, 1);

	glTexCoord2f(1, 1);
	glVertex2f(1, -1);

	glTexCoord2f(0, 1);
	glVertex2f(-1, -1);

	glEnd();

	glDisable(GL_BLEND);
}

void UIManager::initJavascript() {
	mApiObj = mView->CreateGlobalJavascriptObject(WSLit("API"));
	auto infoVal = mView->CreateGlobalJavascriptObject(WSLit("API.info"));
	mFSObj = mView->CreateGlobalJavascriptObject(WSLit("API.filesystem"));
	mModelObj = mView->CreateGlobalJavascriptObject(WSLit("API.model"));
	mAreaObj = mView->CreateGlobalJavascriptObject(WSLit("API.area"));

	JSObject& fsObj = mFSObj.ToObject();
	JSObject& infoObj = infoVal.ToObject();
	JSObject& mdObj = mModelObj.ToObject();
	JSObject& apiObj = mApiObj.ToObject();
	JSObject& arObj = mAreaObj.ToObject();

	infoObj.SetProperty(WSLit("timestamp"), JSValue(WSLit(__TIMESTAMP__)));
#ifdef _DEBUG
	infoObj.SetProperty(WSLit("configuration"), JSValue(WSLit("DEBUG")));
#else
	infoObj.SetProperty(WSLit("configuration"), JSValue(WSLit("RELEASE")));
#endif

	fsObj.SetCustomMethod(WSLit("setPath"), false);
	fsObj.SetCustomMethod(WSLit("getExtension"), true);
	fsObj.SetCustomMethod(WSLit("openTexture"), false);
	fsObj.SetCustomMethod(WSLit("openText"), true);
	fsObj.SetCustomMethod(WSLit("openTextUtf16"), true);
	fsObj.SetCustomMethod(WSLit("openTable"), true);
	fsObj.SetCustomMethod(WSLit("openBinFile"), true);
	fsObj.SetCustomMethod(WSLit("openModel"), false);
	fsObj.SetCustomMethod(WSLit("getFolder"), true);
	fsObj.SetCustomMethod(WSLit("setExtractionFolder"), false);
	fsObj.SetCustomMethod(WSLit("getExtractionFolder"), true);
	fsObj.SetCustomMethod(WSLit("extractEntry"), false);
	fsObj.SetCustomMethod(WSLit("exportTexture"), false);
	fsObj.SetCustomMethod(WSLit("loadI3"), true);
	fsObj.SetCustomMethod(WSLit("loadArea"), true);
	fsObj.SetCustomMethod(WSLit("loadShader"), true);
	fsObj.SetCustomMethod(WSLit("textureAsBmp"), true);
	fsObj.SetCustomMethod(WSLit("setTextureAsBmp"), false);
	fsObj.SetCustomMethod(WSLit("tblAsCsv"), true);
	fsObj.SetCustomMethod(WSLit("tblAsSql"), true);
	fsObj.SetCustomMethod(WSLit("setTblAsCsv"), false);
	fsObj.SetCustomMethod(WSLit("setTblAsSql"), false);
	fsObj.SetCustomMethod(WSLit("setMouseSensitivity"), false);
	fsObj.SetCustomMethod(WSLit("saveScript"), false);
	fsObj.SetCustomMethod(WSLit("openScript"), true);
	fsObj.SetCustomMethod(WSLit("filteredExport"), false);

	mdObj.SetCustomMethod(WSLit("exportObj"), false);
	mdObj.SetCustomMethod(WSLit("pauseRotation"), false);
	mdObj.SetCustomMethod(WSLit("restartRotation"), false);
	mdObj.SetCustomMethod(WSLit("resetRotation"), false);
	mdObj.SetCustomMethod(WSLit("toggleViewMode"), false);
	mdObj.SetCustomMethod(WSLit("toggleNormals"), false);
	mdObj.SetCustomMethod(WSLit("setI3Sector"), false);

	arObj.SetCustomMethod(WSLit("exportObj"), false);

	apiObj.SetCustomMethod(WSLit("openLink"), false);
	apiObj.SetCustomMethod(WSLit("runScript"), false);
	apiObj.SetCustomMethod(WSLit("resetImage"), false);
}

JSValue UIManager::OnMethodCallWithReturnValue(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args) {
	std::wstring method((const wchar_t*) method_name.data(), method_name.length());

	if (remote_object_id == mFSObj.ToObject().remote_id()) {
		if (method == L"getExtension") {
			if (args.size() < 1) {
				return JSValue();
			}

			auto str = args[0].ToString();
			return JSValue(FileHelper::getExtension(str));
		} else if (method == L"openText") {
			if (args.size() < 1) {
				return JSValue::Undefined();
			}

			auto str = args[0].ToString();
			return JSValue(onFileSystemGetContent(str));
		}
		else if (method == L"openTextUtf16")
		{
			if (args.size() < 1)
			{
				return JSValue::Undefined();
			}

			auto str = args[0].ToString();
			return JSValue(onFileSystemGetContentUtf16(str));
		} else if (method == L"openTable") {
			if (args.size() < 1) {
				return JSValue::Undefined();
			}

			WebString str = args[0].ToString();
			std::wstring file((const wchar_t*) str.data(), str.length());
			return JSValue(onFileSystemOpenTable(file));
		} else if (method == L"openBinFile") {
			if (args.size() < 1) {
				return JSValue::Undefined();
			}

			WebString str = args[0].ToString();
			std::wstring file((const wchar_t*) str.data(), str.length());
			return JSValue(onFileSystemOpenBin(file));
		} else if (method == L"getFolder") {
			return JSValue(onFileSystemGetFolder());
		} else if (method == L"getExtractionFolder") {
			return JSValue(WebString((const wchar16*) sIOMgr->getExtractionPath().c_str()));
		} else if (method == L"loadI3") {
			return JSValue(onFSLoadI3Model(args));
		} else if (method == L"textureAsBmp") {
			return JSValue(mTextureAsBmp);
		} else if (method == L"tblAsCsv") {
			return JSValue(mTblAsCsv);
		} else if (method == L"loadArea") {
			return JSValue(onFSLoadArea(args));
		} else if (method == L"loadShader") {
			return JSValue(onFSLoadShader(args));
		} else if (method == L"openScript") {
			return JSValue(onScriptOpen(args));
		} else if (method == L"tblAsSql") {
			return JSValue(mTblAsSql);
		}
	}

	return JSValue();
}

void UIManager::OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args) {
	std::wstring method((const wchar_t*) method_name.data(), method_name.length());

	if (remote_object_id == mFSObj.ToObject().remote_id()) {
		handleFSMethod(method, args);
	} else if (remote_object_id == mModelObj.ToObject().remote_id()) {
		if (method == L"exportObj") {
			if (mActiveModel != nullptr) {
				std::wstring js = L"onAsyncExtractionOperation();";
				mView->ExecuteJavascript(WebString((const wchar16*) js.c_str()), WSLit(""));
				mActiveModel->exportAsObj();
			}
		} else if (method == L"pauseRotation") {
			if (mActiveModel != nullptr) {
				mModelRender.stopRotation();
			}
		} else if (method == L"restartRotation") {
			if (mActiveModel != nullptr) {
				mModelRender.restartRotation();
			}
		} else if (method == L"resetRotation") {
			if (mActiveModel != nullptr) {
				mModelRender.resetRotation();
			}
		} else if (method == L"toggleViewMode") {
			if (mActiveModel != nullptr) {
				mModelRender.toggleViewMode();
			}
		} else if (method == L"toggleNormals") {
			if (mActiveModel != nullptr) {
				mModelRender.toggleNormals();
			}
		} else if (method == L"setI3Sector") {
			if (mActiveI3 != nullptr) {
				if (args.size() < 1) {
					return;
				}

				int32 sector = args[0].ToInteger();
				if ((int64)sector > (int64)mActiveI3->getSectors().size() || sector < 0) {
					return;
				}

				mModelRender.setI3Sector((uint32) sector);
			}
		}
	} else if (remote_object_id == mApiObj.ToObject().remote_id()) {
		if (method == L"openLink") {
			if (args.size() < 1) {
				return;
			}

			auto str = args[0].ToString();
			std::wstring link((const wchar_t*) str.data(), str.length());
			ShellExecute(nullptr, L"open", link.c_str(), L"", L"", SW_SHOW);
		} else if (method == L"runScript") {
			if (args.size() < 1) {
				return;
			}

			auto str = args[0].ToString();
			std::wstring code((const wchar_t*) str.data(), str.length());
			sScriptMgr->getInstance()->run(code);
		} else if (method == L"resetImage") {
			mImageX = mImageY = 0.0f;
		}
	} else if (remote_object_id == mAreaObj.ToObject().remote_id()) {
		if (method == L"exportObj") {
			if (mActiveArea != nullptr) {
				std::wstring js = L"onAsyncExtractionOperation();";
				mView->ExecuteJavascript(WebString((const wchar16*) js.c_str()), WSLit(""));

				mActiveArea->exportToObj();
			}
		}
	}
}

void UIManager::handleFSMethod(const std::wstring& method, const Awesomium::JSArray& args) {
	if (method == L"setPath") {
		onFileSystemSetPath();
	} else if (method == L"openTexture") {
		onFileSystemOpenTexture(args);
	} else if (method == L"openModel") {
		if (args.size() < 1) {
			return;
		}

		auto ws = args[0].ToString();
		std::wstring file((const wchar_t*) ws.data(), ws.length());
		auto entry = sIOMgr->getArchive()->getByPath(file);
		if (entry == nullptr || entry->isDirectory()) {
			return;
		}

		mActiveModel = std::make_shared<M3Model>(file, std::dynamic_pointer_cast<FileEntry>(entry));
		mActiveModel->load();

		mActiveTable = nullptr;
		mActiveTexture = nullptr;
		mActiveI3 = nullptr;
		mActiveBin = nullptr;
	} else if (method == L"setExtractionFolder") {
		if (args.size() < 1) {
			return;
		}

		auto ws = args[0].ToString();
		std::wstring folder((const wchar_t*) ws.data(), ws.length());
		sIOMgr->setExtractionPath(folder);
	} else if (method == L"extractEntry") {
		if (args.size() < 1) {
			return;
		}

		auto ws = args[0].ToString();
		std::wstring entry((const wchar_t*) ws.data(), ws.length());
		std::wstring js = L"onAsyncExtractionOperation();";
		mView->ExecuteJavascript(WebString((const wchar16*) js.c_str()), WSLit(""));
		sIOMgr->extractEntry(entry);
	} else if (method == L"exportTexture") {
		if (mActiveTexture != nullptr) {
			std::wstring js = L"onAsyncExtractionOperation();";
			mView->ExecuteJavascript(WebString((const wchar16*) js.c_str()), WSLit(""));
			mActiveTexture->exportAsPng();
		}
	} else if (method == L"setTextureAsBmp") {
		if (args.size() < 1) {
			return;
		}

		setTextureAsBmp(args[0].ToBoolean());
	} else if (method == L"setTblAsCsv") {
		if (args.size() < 1) {
			return;
		}

		setTblAsCsv(args[0].ToBoolean());
	} else if (method == L"setTblAsSql") {
		if (args.size() < 1) {
			return;
		}

		setTblAsSql(args[0].ToBoolean());
	} else if (method == L"setMouseSensitivity") {
		if (args.size() < 1) {
			return;
		}
		mMouseSensitivity = static_cast<float>(args[0].ToDouble());
	} else if (method == L"saveScript") {
		if (args.size() < 1) {
			return;
		}

		auto ws = args[0].ToString();
		std::wstring code((const wchar_t*) ws.data(), ws.length());
		IFileSaveDialog* dlg = nullptr;
		HRESULT hRes = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dlg));
		if (FAILED(hRes) || dlg == nullptr) {
			return;
		}

		COMDLG_FILTERSPEC filter = {
			L"Javascript-File", L"*.js"
		};

		dlg->SetFileTypes(1, &filter);
		dlg->SetFileName(L"");
		dlg->SetDefaultExtension(L"js");

		hRes = dlg->Show(mWindow->getHandle());
		if (SUCCEEDED(hRes)) {
			IShellItem* resItem = nullptr;
			dlg->GetResult(&resItem);
			if (resItem != nullptr) {
				LPWSTR lpPath = nullptr;
				resItem->GetDisplayName(SIGDN_FILESYSPATH, &lpPath);
				resItem->Release();
				if (lpPath != nullptr) {
					std::wofstream os(lpPath);
					os << code;
					os.close();
					CoTaskMemFree(lpPath);
				}
			}
		}

		dlg->Release();
	} else if (method == L"filteredExport") {
		onFilteredExport(args);
	}
}

void UIManager::onFilteredExport(const JSArray& args) {
	if (args.size() < 1) {
		return;
	}

	JSObject obj = args[0].ToObject();
	JSArray arr = obj.GetProperty(WSLit("extensions")).ToArray();
	auto regex = obj.GetProperty(WSLit("regex")).ToString();
	auto areaAsObj = obj.GetProperty(WSLit("areaAsObj")).ToBoolean();
	auto texAsBmp = obj.GetProperty(WSLit("texAsBmp")).ToBoolean();
	auto m3AsObj = obj.GetProperty(WSLit("m3AsObj")).ToBoolean();
	auto tblAsCsv = obj.GetProperty(WSLit("tblAsCsv")).ToBoolean();
	auto entry = obj.GetProperty(WSLit("entry")).ToString();

	FilterParameters params;
	params.areaAsObj = areaAsObj;
	params.m3AsObj = m3AsObj;
	params.regex = std::wstring((const wchar_t*) regex.data(), regex.length());
	params.tblAsCsv = tblAsCsv;
	params.texAsBmp = texAsBmp;

	for (uint32 i = 0; i < arr.size(); ++i) {
		auto str = arr[i].ToString();
		params.extensions.push_back(std::wstring((const wchar_t*) str.data(), str.length()));
		auto& extStr = params.extensions.back();
		if (extStr.find(L'.') != 0) {
			extStr = std::wstring(L".") + extStr;
		}

		std::transform(extStr.begin(), extStr.end(), extStr.begin(), std::towupper);
	}

	sIOMgr->extractEntriesByFilter(std::wstring((const wchar_t*) entry.data(), entry.length()), params);
}

void UIManager::setTblAsSql(bool value) {
	mTblAsSql = value;

	HKEY baseKey = nullptr;
	auto res = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Cromon\\WildstarStudio", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &baseKey, nullptr);
	if (res != ERROR_SUCCESS) {
		return;
	}

	uint32 regValue = value ? 1 : 0;

	RegSetValueEx(baseKey, L"TblAsSql", 0, REG_DWORD, (LPCBYTE) &value, sizeof(uint32));
}

void UIManager::setTblAsCsv(bool value) {
	mTblAsCsv = value;

	HKEY baseKey = nullptr;
	auto res = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Cromon\\WildstarStudio", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &baseKey, nullptr);
	if (res != ERROR_SUCCESS) {
		return;
	}

	uint32 regValue = value ? 1 : 0;

	RegSetValueEx(baseKey, L"TblAsCsv", 0, REG_DWORD, (LPCBYTE) &value, sizeof(uint32));
}

void UIManager::setTextureAsBmp(bool value) {
	mTextureAsBmp = value;

	HKEY baseKey = nullptr;
	auto res = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Cromon\\WildstarStudio", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &baseKey, nullptr);
	if (res != ERROR_SUCCESS) {
		return;
	}

	uint32 regValue = value ? 1 : 0;

	RegSetValueEx(baseKey, L"TextureAsBmp", 0, REG_DWORD, (LPCBYTE) &value, sizeof(uint32));
}

JSValue UIManager::onScriptOpen(const JSArray& args) {
	IFileOpenDialog* dlg = nullptr;
	auto res = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dlg));
	if (FAILED(res) || dlg == nullptr) {
		return JSValue::Null();
	}

	COMDLG_FILTERSPEC filter = {
		L"Javascript-File", L"*.js"
	};

	dlg->SetFileTypes(1, &filter);
	dlg->SetFileName(L"");
	dlg->SetDefaultExtension(L"js");
	
	res = dlg->Show(mWindow->getHandle());
	if (SUCCEEDED(res)) {
		IShellItem* resItem = nullptr;
		dlg->GetResult(&resItem);
		if (resItem != nullptr) {
			LPWSTR lpPath = nullptr;
			resItem->GetDisplayName(SIGDN_FILESYSPATH, &lpPath);
			resItem->Release();
			if (lpPath != nullptr) {
				std::wifstream is(lpPath);
				if (is.is_open() == false) {
					return JSValue::Null();
				}

				std::wstringstream content;
				std::wstring line;
				while (std::getline(is, line)) {
					content << line << std::endl;
				}
				CoTaskMemFree(lpPath);
				dlg->Release();
				return WebString((const wchar16*) content.str().c_str(), content.str().length());
			}
		}
	}

	dlg->Release();
	return WSLit("");
}

WebString UIManager::onFSLoadShader(const JSArray& args) {
	if (args.size() < 1) {
		return WSLit("");
	}

	auto ws = args[0].ToString();
	std::wstring file((const wchar_t*) ws.data(), ws.length());
	auto entry = sIOMgr->getArchive()->getByPath(file);
	if (entry == nullptr || entry->isDirectory()) {
		return WSLit("");
	}

	Shader shader(std::dynamic_pointer_cast<FileEntry>(entry));
	if (shader.load() == false) {
		return WSLit("[ ]");
	}

	std::wstringstream strm;
	strm << L"[";
	bool first = true;
	for (auto& perm : shader.getPermutations()) {
		if (first == true) {
			first = false;
		} else {
			strm << L", ";
		}

		strm << L"\"" << escapeJsonString(perm) << L"\"";
	}
	strm << L"]";

	return WebString((const wchar16*) strm.str().c_str());
}

WebString UIManager::onFSLoadArea(const JSArray& args) {
	if (args.size() < 1) {
		return WSLit("");
	}

	auto ws = args[0].ToString();
	std::wstring file((const wchar_t*) ws.data(), ws.length());
	auto entry = sIOMgr->getArchive()->getByPath(file);
	if (entry == nullptr || entry->isDirectory()) {
		return WSLit("");
	}

	mActiveArea = std::make_shared<AreaFile>(file, std::dynamic_pointer_cast<FileEntry>(entry));
	mActiveArea->load();

	mActiveTable = nullptr;
	mActiveTexture = nullptr;
	mActiveModel = nullptr;
	mActiveBin = nullptr;
	mActiveI3 = nullptr;

	mAreaRender.onModelSelected(mActiveArea);

	return WSLit("");
}

WebString UIManager::onFSLoadI3Model(const JSArray& args) {
	if (args.size() < 2) {
		return WSLit("");
	}

	JSValue cbVal = args[1];
	JSObject& cbObj = cbVal.ToObject();

	auto ws = args[0].ToString();
	std::wstring file((const wchar_t*) ws.data(), ws.length());
	auto entry = sIOMgr->getArchive()->getByPath(file);
	if (entry == nullptr || entry->isDirectory()) {
		return WSLit("");
	}

	mModelRender.setI3Sector(0);

	mActiveI3 = std::make_shared<I3Model>(std::dynamic_pointer_cast<FileEntry>(entry));
	if (mActiveI3->load() == false) {
		mActiveI3 = nullptr;
		return WSLit("");
	}

	mActiveTable = nullptr;
	mActiveTexture = nullptr;
	mActiveModel = nullptr;
	mActiveBin = nullptr;

	std::wstringstream strm;
	strm << L"{ \"passes\": [ ";
	for (uint32 i = 0; i < mActiveI3->getNumPasses(); ++i) {
		if (i != 0) {
			strm << L", ";
		}
		strm << L"{ \"name\": \"" << mActiveI3->getPass(i)->getPassName() << L"\", \"textures\": [ ";
		for (uint32 j = 0; j < mActiveI3->getPass(i)->getNumTextures(); ++j) {
			//WebString wTexStr = WSLit(String::toAnsi(std::wstring(mActiveI3->getPass(i)->getTexUnit(j).textureName, (uint32) mActiveI3->getPass(i)->getTexUnit(j).lenTextureName)).c_str());
			std::wstring texStr = L"";//((const wchar_t*)wTexStr.data(), wTexStr.length());
			std::replace(texStr.begin(), texStr.end(), L'\\', L'/');
			if (j != 0) {
				strm << L", ";
			}
			strm << L"\"" << texStr << L"\"";
		}

		strm << L" ] } ";
	}

	strm << L" ], \"sectors\": [";

	for (uint32 i = 0; i < mActiveI3->getSectors().size(); ++i) {
		if (i != 0) {
			strm << L", ";
		}

		strm << L" { \"name\": \"" << mActiveI3->getSectors()[i]->getName() << L"\", \"id\": " << i << L" }";
	}

	strm << L" ] }";

	return WebString((const wchar16*) strm.str().c_str());
}

WebString UIManager::onFileSystemGetFolder() {
	std::vector<wchar_t> folderBuffer(MAX_PATH + 1);

	BROWSEINFO bwi = { 0 };
	bwi.hwndOwner = mWindow->getHandle();
	bwi.pidlRoot = nullptr;
	bwi.pszDisplayName = folderBuffer.data();
	bwi.lpszTitle = L"Select folder";
	bwi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	LPITEMIDLIST resp = SHBrowseForFolder(&bwi);
	if (resp == nullptr) {
		return WSLit("");
	}

	if (SHGetPathFromIDList(resp, folderBuffer.data()) == FALSE) {
		return WSLit("");
	}

	return WebString((const wchar16*) folderBuffer.data());
}

WebString UIManager::onFileSystemOpenBin(const std::wstring& bin) {
	auto entry = sIOMgr->getArchive()->getByPath(bin);
	if (entry == nullptr || entry->isDirectory()) {
		return WSLit("{ }");
	}

	mActiveModel = nullptr;
	mActiveTexture = nullptr;
	mActiveI3 = nullptr;
	mActiveTable = nullptr;

	mActiveBin = std::make_shared<BinLocale>(std::dynamic_pointer_cast<FileEntry>(entry));
	mActiveBin->load();
	std::wstringstream info;
	info << L"{ \"numEntries\": " << mActiveBin->getHeader().numEntries << L" }";
	return WSLit(String::toAnsi(info.str()).c_str());
}

WebString UIManager::onFileSystemOpenTable(const std::wstring& table) {
	auto entry = sIOMgr->getArchive()->getByPath(table);
	if (entry == nullptr || entry->isDirectory()) {
		return WSLit("{ }");
	}
	
	mActiveModel = nullptr;
	mActiveTexture = nullptr;
	mActiveI3 = nullptr;
	mActiveBin = nullptr;

	mActiveTable = std::make_shared<DataTable>(std::dynamic_pointer_cast<FileEntry>(entry));
	mActiveTable->initialLoad();
	std::wstring scheme = mActiveTable->createScheme();
	std::wstringstream info;
	info << L"{ \"numEntries\": " << mActiveTable->numEntries() << L", \"columns\": " << scheme << L" }";
	return WSLit(String::toAnsi(info.str()).c_str());
}

WebString UIManager::onFileSystemGetContent(const Awesomium::WebString& str) {
	std::wstring fname((const wchar_t*)str.data(), str.length());
	auto entry = sIOMgr->getArchive()->getByPath(fname);
	if (entry->isDirectory()) {
		return WSLit("");
	}

	auto fentry = std::dynamic_pointer_cast<FileEntry>(entry);
	std::vector<uint8> content;
	sIOMgr->getArchive()->getFileData(fentry, content);
	std::vector<char> strData(content.size() + 2, 0);
	std::copy(content.begin(), content.end(), strData.begin());

	return WSLit(strData.data());
}

WebString UIManager::onFileSystemGetContentUtf16(const Awesomium::WebString& str) {
	std::wstring fname((const wchar_t*)str.data(), str.length());
	auto entry = sIOMgr->getArchive()->getByPath(fname);
	if (entry->isDirectory()) {
		return WSLit("");
	}

	auto fentry = std::dynamic_pointer_cast<FileEntry>(entry);
	std::vector<uint8> content;
	sIOMgr->getArchive()->getFileData(fentry, content);

	return WebString(reinterpret_cast<wchar16*>(content.data()), static_cast<unsigned int>(content.size() / 2));
}

void UIManager::onFileSystemOpenTexture(const Awesomium::JSArray& args) {
	if (args.size() < 1) {
		return;
	}

	auto texName = args[0].ToString();
	auto cbVal = args[1];
	auto& callback = cbVal.ToObject();
	auto propVal = args[2];
	auto& pobj = propVal.ToObject();

	auto entry = sIOMgr->getArchive()->getByPath(std::wstring((const wchar_t*) texName.data()));
	if (entry == nullptr || entry->isDirectory()) {
		return;
	}

	mActiveTexture = std::make_shared<Texture>(std::dynamic_pointer_cast<FileEntry>(entry));
	mActiveTexture->loadTexture();

	mActiveI3 = nullptr;
	mActiveModel = nullptr;
	mActiveTable = nullptr;
	mActiveBin = nullptr;

	pobj.SetProperty(WSLit("width"), JSValue((int32) mActiveTexture->getHeader().width));
	pobj.SetProperty(WSLit("height"), JSValue((int32) mActiveTexture->getHeader().height));
	pobj.SetProperty(WSLit("formatEntry"), JSValue((int32) mActiveTexture->getHeader().texFormatIndex));

	JSArray arr(1);
	arr.Insert(pobj, 0);

	callback.Invoke(WSLit("onPropertiesLoaded"), arr);
}

void UIManager::onFileSystemSetPath() {
	IFileOpenDialog* openDlg;
	HRESULT hRes = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&openDlg));
	if (FAILED(hRes)) {
		return;
	}

	COMDLG_FILTERSPEC filter = { 0 };
	filter.pszName = L"ClientData.index";
	filter.pszSpec = L"*.index";

	openDlg->SetFileTypes(1, &filter);

	if (FAILED(openDlg->Show(mWindow->getHandle()))) {
		openDlg->Release();
		return;
	}

	IShellItem* resItem = nullptr;
	if (FAILED(openDlg->GetResult(&resItem))) {
		openDlg->Release();
		return;
	}

	openDlg->Release();

	LPWSTR filePath = nullptr;
	if (FAILED(resItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath))) {
		resItem->Release();
		return;
	}

	resItem->Release();

	if (filePath != nullptr) {
		std::wstringstream strm;
		std::wstring path = filePath;

		strm << L"onDataFolderChanged('";
		
		std::wstring::size_type t = path.find(L'\\');
		std::wstring::size_type lastPos = 0;
		while (t != std::wstring::npos) {
			strm << path.substr(lastPos, t - lastPos) << L"\\\\";
			lastPos = t + 1;
			t = path.find(L'\\', t + 1);
		}

		strm << path.substr(lastPos) << L"');";

		CoTaskMemFree(filePath);
		std::wstring code = strm.str();
		
		auto str = WebString((const wchar16*) code.c_str());
		mView->ExecuteJavascript(str, WSLit(""));

		sIOMgr->loadFromPath(path);

		strm = std::wstringstream();
		strm << L"onUpdateFileLoadProgress(0, " << sIOMgr->getArchive()->getFileCount() << ");";
		mView->ExecuteJavascript(WebString((const wchar16*) strm.str().c_str()), WSLit(""));

		sIOMgr->doAsyncLoad([this]() { mIsAsyncLoadComplete = true; });
	}
}

void UIManager::asyncUpdate() {
	if (sIOMgr->isAsyncUpdating()) {
		std::wstringstream strm;
		strm << L"onUpdateFileLoadProgress(" << sIOMgr->getAsyncLoadCount() << ", " << sIOMgr->getArchive()->getFileCount() << ");";
		mView->ExecuteJavascript(WebString((const wchar16*) strm.str().c_str()), WSLit(""));
	}

	if (mIsAsyncLoadComplete == true) {
		std::wstringstream strm;
		strm << L"onFileLoadDone([";
		std::list<IFileSystemEntryPtr> root;
		sIOMgr->getArchive()->getRoot()->getEntries(root);

		bool first = true;
		for (auto& entry : root) {
			if (first == true) {
				first = false;
			} else {
				strm << L", ";
			}

			strm << L"{ title: '";
			strm << entry->getEntryName() << L"', key: '";
			std::wstring path = entry->getFullPath();
			std::wstring::size_type t = path.find(L'\\');
			std::wstring::size_type lastPos = 0;
			while (t != std::wstring::npos) {
				strm << path.substr(lastPos, t - lastPos) << L"\\\\";
				lastPos = t + 1;
				t = path.find(L'\\', t + 1);
			}

			strm << path.substr(lastPos);

			strm << L"', isFolder: " << (entry->isDirectory() ? L"true" : L"false");
			if (entry->isDirectory()) {
				strm << L", isLazy: true";
			}

			strm << L" }";
		}

		strm << "]); ";
		mView->ExecuteJavascript(WebString((const wchar16*) strm.str().c_str()), WSLit(""));

		mIsAsyncLoadComplete = false;
	}
}

void UIManager::onFileSystemRequest(int request_id, const WebString& path) {
	std::wstring fp((const wchar_t*) path.data(), path.length());
	fp = fp.substr(9);

	if (fp.find(L"get_child.json") == 0 && fp.find(L"?key=") != std::wstring::npos) {
		std::wstring::size_type keyStart = fp.find(L"?key=");
		std::wstring::size_type keyEnd = fp.find(L"&", keyStart + 1);
		std::wstring key = fp.substr(keyStart + 5, keyEnd != std::wstring::npos ? (keyEnd - keyStart - 5) : std::wstring::npos);

		auto entry = sIOMgr->getArchive()->getByPath(key);
		std::wstringstream strm;
		strm << L"[ ";

		if (entry->isDirectory() == true) {
			std::list<IFileSystemEntryPtr> children;
			std::dynamic_pointer_cast<DirectoryEntry>(entry)->getEntries(children);

			bool first = true;
			for (auto& entry : children) {
				if (first == true) {
					first = false;
				} else {
					strm << L", ";
				}

				strm << L"{ \"title\": \"";
				strm << entry->getEntryName() << L"\", \"key\": \"";
				std::wstring path = entry->getFullPath();
				std::wstring::size_type t = path.find(L'\\');
				std::wstring::size_type lastPos = 0;
				while (t != std::wstring::npos) {
					strm << path.substr(lastPos, t - lastPos) << L"\\\\";
					lastPos = t + 1;
					t = path.find(L'\\', t + 1);
				}

				strm << path.substr(lastPos);

				strm << L"\" ";
				if (entry->isDirectory()) {
					strm << L", \"isFolder\": \"true\", \"isLazy\": \"true\"";
				} else {
					auto ext = std::tr2::sys::path(path).extension().wstring();
					ext = ext.substr(1);
					std::wstring fullPath = L"UI\\utils\\skin\\";
					fullPath += ext + std::wstring(L".png");
					if (exists(std::tr2::sys::path(fullPath))) {
						strm << L", \"icon\": \"" << ext << ".png\" ";
					}
				}

				strm << L" }";
			}
		}

		strm << " ]";

		std::string resp = String::toAnsi(strm.str());

		SendResponse(request_id, resp.length(), (unsigned char*) resp.data(), WSLit("text/json"));
		return;
	}

	if (fp.find(L"tbl.json") == 0) {
		handleTblQuery(request_id, fp);
	} else if (fp.find(L"bin.json") == 0) {
		handleBinQuery(request_id, fp);
	} else if (fp.find(L"get_image") == 0) {
		handleImageQuery(request_id, fp);
	}
}

void UIManager::handleBinQuery(int request_id, const std::wstring& path) {
	std::wstring qry = path.substr(9);
	std::list<std::wstring> parts;
	String::split(qry, parts, L'&');

	uint32 page = 0;
	uint32 numEntries = 0;
	uint32 findMask = 0;

	for (auto& str : parts) {
		std::list<std::wstring> subParts;
		String::split(str, subParts, L'=');
		if (subParts.size() < 2) {
			continue;
		}

		std::wstring name = *subParts.begin();
		std::wstring value = *(++subParts.begin());

		name = String::toLower(name);
		if (name == L"page") {
			std::wstringstream strm;
			strm << value;
			if (strm >> page) {
				findMask |= 1;
			}
		} else if (name == L"per_page") {
			std::wstringstream strm;
			strm << value;
			if (strm >> numEntries) {
				findMask |= 2;
			}
		}
	}

	if (findMask != 3) {
		SendResponse(request_id, 0, nullptr, WSLit(""));
		return;
	}

	if (mActiveBin == nullptr) {
		SendResponse(request_id, 0, nullptr, WSLit(""));
	}

	uint32 start = (page - 1) * 27;
	uint32 end = start + numEntries;

	std::string response = String::toAnsi(mActiveBin->loadRange(start, end));
	SendResponse(request_id, response.length(), (unsigned char*) response.c_str(), WSLit("text/json"));
}

void UIManager::handleTblQuery(int request_id, const std::wstring& path) {
	std::wstring qry = path.substr(9);
	std::list<std::wstring> parts;
	String::split(qry, parts, L'&');

	uint32 page = 0;
	uint32 numEntries = 0;
	uint32 findMask = 0;

	for (auto& str : parts) {
		std::list<std::wstring> subParts;
		String::split(str, subParts, L'=');
		if (subParts.size() < 2) {
			continue;
		}

		std::wstring name = *subParts.begin();
		std::wstring value = *(++subParts.begin());

		name = String::toLower(name);
		if (name == L"page") {
			std::wstringstream strm;
			strm << value;
			if (strm >> page) {
				findMask |= 1;
			}
		} else if (name == L"per_page") {
			std::wstringstream strm;
			strm << value;
			if (strm >> numEntries) {
				findMask |= 2;
			}
		}
	}

	if (findMask != 3) {
		SendResponse(request_id, 0, nullptr, WSLit(""));
		return;
	}

	if (mActiveTable == nullptr) {
		SendResponse(request_id, 0, nullptr, WSLit(""));
	}

	uint32 start = (page - 1) * 27;
	uint32 end = start + numEntries;

	std::string response = String::toAnsi(mActiveTable->loadRange(start, end));
	SendResponse(request_id, response.length(), (unsigned char*) response.c_str(), WSLit("text/json"));
}

void UIManager::handleImageQuery(int request_id, const std::wstring& _path) {
	auto pathStart = _path.find(L"?path=");
	auto path = _path.substr(pathStart + strlen("?path="));

	auto entry = std::dynamic_pointer_cast<FileEntry>(sIOMgr->getArchive()->getByPath(path));
	if (!entry)
		return;

	std::vector<uint8> mContent;
	sIOMgr->getArchive()->getFileData(entry, mContent);

	auto extension = FileHelper::getExtension(path);
	WebString mimetype;
	if (extension == L"jpg") {
		mimetype = WSLit("image/jpeg");
	} else if (extension == L"bmp") {
		mimetype = WSLit("image/bmp");
	}

	SendResponse(request_id, mContent.size(), reinterpret_cast<unsigned char*>(const_cast<uint8*>(mContent.data())), mimetype);

}

void UIManager::OnChangeCursor(WebView* caller, Cursor cursor) {
	mCursor = cursor;

	switch (cursor) {
	case Cursor::kCursor_Hand:
		SetCursor(LoadCursor(nullptr, IDC_HAND));
		break;

	case Cursor::kCursor_VerticalText:
		SetCursor(LoadCursor(nullptr, IDC_IBEAM));
		break;

	default:
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		break;
	}
}

void UIManager::pushSyncAction(std::function<void ()> fun) {
	std::lock_guard<std::mutex> l(mSyncFrameLock);

	mSyncFrames.push_back(fun);
}

void UIManager::asyncExtractComplete() {
	pushSyncAction(
		[this]() {
			mView->ExecuteJavascript(WSLit("onAsyncExtractionDone();"), WSLit(""));
		}
	);
}

void UIManager::initRegistryDefault() {
	mTextureAsBmp = false;
}

void UIManager::initRegistryValues() {
	HKEY hBaseKey = nullptr;
	auto res = RegOpenKey(HKEY_CURRENT_USER, L"Software\\Cromon\\WildstarStudio", &hBaseKey);
	if (res != ERROR_SUCCESS) {
		return initRegistryDefault();
	}

	uint32 regValue = 0;
	uint32 type = REG_DWORD;
	uint32 numData = sizeof(uint32);
	res = RegQueryValueEx(hBaseKey, L"TextureAsBmp", nullptr, (LPDWORD) &type, (LPBYTE) &regValue, (LPDWORD) &numData);
	if (res != ERROR_SUCCESS) {
		mTextureAsBmp = false;
	} else {
		mTextureAsBmp = regValue != 0;
	}

	res = RegQueryValueEx(hBaseKey, L"TblAsCsv", nullptr, (LPDWORD) &type, (LPBYTE) &regValue, (LPDWORD) &numData);
	if (res != ERROR_SUCCESS) {
		mTblAsCsv = false;
	} else {
		mTblAsCsv = regValue != 0;
	}

	res = RegQueryValueEx(hBaseKey, L"TblAsSql", nullptr, (LPDWORD) &type, (LPBYTE) &regValue, (LPDWORD) &numData);
	if (res != ERROR_SUCCESS) {
		mTblAsSql = false;
	} else {
		mTblAsSql = regValue != 0;
	}
}

void UIManager::executeJavascript(const std::wstring& code) {
	mView->ExecuteJavascript(WebString((const wchar16*) code.c_str(), code.length()), WSLit(""));
}

void UIManager::initWwise() {

}

namespace AK
{
	void * AllocHook(size_t in_size)
	{
		return malloc(in_size);
	}

	void FreeHook(void * in_ptr)
	{
		free(in_ptr);
	}

	void * VirtualAllocHook(
		void * in_pMemAddress,
		size_t in_size,
		DWORD in_dwAllocationType,
		DWORD in_dwProtect
		)
	{
		return VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
	}

	void VirtualFreeHook(
		void * in_pMemAddress,
		size_t in_size,
		DWORD in_dwFreeType
		)
	{
		VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
	}

}