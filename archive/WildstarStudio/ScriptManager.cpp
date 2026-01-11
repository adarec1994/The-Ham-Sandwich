#include "stdafx.h"
#include "ScriptManager.h"
#include "Files.h"
#include "Common.h"
#include "AreaJS.h"
#include "UIManager.h"

std::shared_ptr<ScriptManager> ScriptManager::gInstance;

ScriptManager::ScriptManager() {
	mGlobalInstance = std::make_shared<V8Instance>();
	v8::V8::SetArrayBufferAllocator(this);
}

void ScriptManager::initGlobalContext() {
	mGlobalInstance->initGlobal();

	FileLibrary::onRegister(mGlobalInstance->getGlobalScope());
	Common::onRegister(mGlobalInstance->getGlobalScope());
	AreaJS::onRegister(mGlobalInstance->getGlobalScope());

	mGlobalInstance->createContexts();
}

void ScriptManager::run(const std::wstring& code) {
	v8::TryCatch tc;
	mGlobalInstance->runScriptGlobal(code);
	if (tc.HasCaught()) {
		std::wstringstream strm;
		auto str = tc.StackTrace()->ToString();
		std::vector<wchar_t> buffer(str->Length());
		str->Write((uint16_t*) buffer.data(), 0, str->Length());
		buffer.push_back(L'\0');

		strm << L"logConsoleMessage(\"Uncaught exception: " << escapeJsonString(buffer.data()) << L"\");";
		sUIMgr->executeJavascript(strm.str());
	}

	v8::V8::IdleNotification();
}

void* ScriptManager::Allocate(size_t len) {
	auto ret = new uint8[len];
	ZeroMemory(ret, len);
	return ret;
}

void* ScriptManager::AllocateUninitialized(size_t len) {
	return new uint8[len];
}

void ScriptManager::Free(void* data, size_t len) {
	delete [] (uint8*)data;
}