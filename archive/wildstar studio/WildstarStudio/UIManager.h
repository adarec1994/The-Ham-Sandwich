#pragma once

#include "Window.h"
#include "Texture.h"
#include "DataTable.h"
#include "M3Model.h"
#include "ModelRender.h"
#include "FileLoaderThread.h"
#include "I3Model.h"
#include "BinLocale.h"
#include "AreaFile.h"
#include "AreaRender.h"

SHARED_FWD(UIManager);

std::wstring escapeJsonString(const std::wstring& input);

class UIManager : public Awesomium::DataSource, public Awesomium::WebViewListener::View, public Awesomium::JSMethodHandler
{
	class FileSystemRequestHandler;

	static UIManagerPtr gInstance;

	Awesomium::WebView* mView;
	Awesomium::WebCore* mCore;
	Awesomium::JSValue mApiObj;
	Awesomium::JSValue mFSObj;
	Awesomium::JSValue mModelObj;
	Awesomium::JSValue mAreaObj;
	Awesomium::Cursor mCursor = Awesomium::Cursor::kCursor_Pointer;
	GLuint mTexture;
	uint32 mWidth;
	uint32 mHeight;
	WindowPtr mWindow;
	std::atomic_bool mIsAsyncLoadComplete;
	TexturePtr mActiveTexture;
	DataTablePtr mActiveTable;
	M3ModelPtr mActiveModel;
	I3ModelPtr mActiveI3;
	BinLocalePtr mActiveBin;
	AreaFilePtr mActiveArea;
	FileLoaderThread mLoadThread;
	std::list<std::function<void ()>> mSyncFrames;
	std::mutex mSyncFrameLock;
	bool mTextureAsBmp;
	bool mTblAsCsv;
	bool mTblAsSql;

	bool mLMouseDown = false;
	float mMouseSensitivity = 1.0f;

	float mImageX = 0.0f;
	float mImageY = 0.0f;

	ModelRender mModelRender;
	AreaRender mAreaRender;

	void initWwise();

	void OnRequest(int request_id, const Awesomium::WebString& path);

	virtual void OnChangeTitle(Awesomium::WebView* caller, const Awesomium::WebString& title) { }
	virtual void OnChangeAddressBar(Awesomium::WebView* caller, const Awesomium::WebURL& url) { }
	virtual void OnChangeTooltip(Awesomium::WebView* caller, const Awesomium::WebString& tooltip) { }
	virtual void OnChangeTargetURL(Awesomium::WebView* caller, const Awesomium::WebURL& url) { }
	virtual void OnChangeCursor(Awesomium::WebView* caller, Awesomium::Cursor cursor);
	virtual void OnChangeFocus(Awesomium::WebView* caller, Awesomium::FocusedElementType focused_type) { }
	virtual void OnAddConsoleMessage(Awesomium::WebView* caller, const Awesomium::WebString& message, int line_number, const Awesomium::WebString& source);
	virtual void OnShowCreatedWebView(Awesomium::WebView* caller, Awesomium::WebView* new_view, const Awesomium::WebURL& opener_url, const Awesomium::WebURL& target_url, const Awesomium::Rect& initial_pos, bool is_popup) { }

	void OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args);
	Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args);

	void handleFSMethod(const std::wstring& method_name, const Awesomium::JSArray& args);
	void onFileSystemSetPath();
	void onFileSystemOpenTexture(const Awesomium::JSArray& args);
	void onFilteredExport(const Awesomium::JSArray& args);
	Awesomium::JSValue onScriptOpen(const Awesomium::JSArray& args);
	Awesomium::WebString onFileSystemOpenTable(const std::wstring& table);
	Awesomium::WebString onFileSystemOpenBin(const std::wstring& binFile);
	Awesomium::WebString onFileSystemGetContent(const Awesomium::WebString& file);
	Awesomium::WebString onFileSystemGetContentUtf16(const Awesomium::WebString& file);
	Awesomium::WebString onFileSystemGetFolder();
	Awesomium::WebString onFSLoadI3Model(const Awesomium::JSArray& args);
	Awesomium::WebString onFSLoadArea(const Awesomium::JSArray& args);
	Awesomium::WebString onFSLoadShader(const Awesomium::JSArray& args);

	void onFileSystemRequest(int request_id, const Awesomium::WebString& path);

	void handleTblQuery(int request_id, const std::wstring& path);
	void handleBinQuery(int request_id, const std::wstring& path);
	void handleImageQuery(int request_id, const std::wstring& path);

	void asyncUpdate();

	void setTextureAsBmp(bool value);
	void setTblAsCsv(bool value);
	void setTblAsSql(bool value);

	void initRegistryValues();
	void initRegistryDefault();
	void initJavascript();
public:
	UIManager();

	UIManager(const UIManager&) = delete;
	void operator = (const UIManager&) = delete;

	~UIManager();

	void init(uint32 width, uint32 height, WindowPtr wnd);
	void shutdown();

	void onFrame();
	void asyncExtractComplete();

	bool extractTextureAsBmp() const { return mTextureAsBmp; }
	bool extractTblAsCsv() const { return mTblAsCsv; }
	bool extractTblAsSql() const { return mTblAsSql; }

	void pushSyncAction(std::function<void ()> fun);

	void executeJavascript(const std::wstring& code);

	WindowPtr getWindow() const { return mWindow; }

	float getAspect() const { return mWindow->getWidth() / (float) mWindow->getHeight(); }

	static UIManagerPtr getInstance() {
		if (gInstance == nullptr) {
			gInstance = std::make_shared<UIManager>();
		}

		return gInstance;
	}
};

#define sUIMgr (UIManager::getInstance())