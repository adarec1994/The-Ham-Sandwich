#pragma once

#include "ObjectWrap.h"
#include "Scope.h"

class V8Instance
{
	class ScopeWrap
	{
		v8::HandleScope mHandleScope;
	public:
		ScopeWrap(v8::Isolate* isolate) : mHandleScope(isolate) {

		}
	};

	v8::Isolate* mIsolate;
	v8::Persistent<v8::Context> mPersContext;
	v8::Handle<v8::Context> mContext;

	std::unique_ptr<v8::Isolate::Scope> mScope;
	std::unique_ptr<v8::Context::Scope> mGlobalContext;
	std::unique_ptr<ScopeWrap> mHandleScope;

	std::shared_ptr<Scope> mGlobalScope;

	v8::Handle<v8::Script> compileScript(const std::wstring& code);
public:
	V8Instance();
	~V8Instance();

	void initGlobal();
	void createContexts();

	void runScriptIsolated(const std::wstring& code);
	void runScriptGlobal(const std::wstring& code);

	template<typename T>
	T getGlobal(const std::wstring& name) {
		v8::HandleScope scope(mIsolate);

		auto value = mContext->Global()->Get(v8::String::New((const uint16_t*) name.c_str(), name.length()));
		if (value.IsEmpty() == true) {
			throw std::exception("Global variable not found!");
		}

		return ObjectWrap::unwrap<T>(value);
	}

	Scope& getGlobalScope() { return *mGlobalScope; }

	v8::Isolate* getIsolate() const { return mIsolate; }
};

SHARED_TYPE(V8Instance);