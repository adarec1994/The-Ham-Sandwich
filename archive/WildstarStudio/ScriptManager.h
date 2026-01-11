#pragma once

#include "V8Instance.h"

class ScriptManager : public v8::ArrayBuffer::Allocator
{
	static std::shared_ptr<ScriptManager> gInstance;

	std::shared_ptr<V8Instance> mGlobalInstance;

	virtual void* Allocate(size_t length);
	virtual void* AllocateUninitialized(size_t length);
	virtual void Free(void* data, size_t length);

public:
	ScriptManager();

	void initGlobalContext();

	void run(const std::wstring& code);

	template<typename T>
	T getGlobal(const std::wstring& name) {
		return mGlobalInstance->getGlobal<T>(name);
	}

	static std::shared_ptr<ScriptManager> getInstance() {
		if (gInstance == nullptr) {
			gInstance = std::make_shared<ScriptManager>();
		}

		return gInstance;
	}
};

#define sScriptMgr (ScriptManager::getInstance())