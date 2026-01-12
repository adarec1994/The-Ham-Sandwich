#include "stdafx.h"
#include "V8Instance.h"
#include "Class.h"

V8Instance::V8Instance() {
	mIsolate = nullptr;
}

V8Instance::~V8Instance() {
	mGlobalContext.reset();
	mHandleScope.reset();
	mScope.reset();

	if (mIsolate != nullptr) {
		mIsolate->Dispose();
	}
}

void V8Instance::initGlobal() {
	mIsolate = v8::Isolate::New();
	mScope = std::unique_ptr<v8::Isolate::Scope>(new v8::Isolate::Scope(mIsolate));

	mHandleScope = std::unique_ptr<ScopeWrap>(new ScopeWrap(mIsolate));

	v8::Handle<v8::ObjectTemplate> globalTemplate = v8::ObjectTemplate::New();
	mGlobalScope = std::make_shared<Scope>(globalTemplate);
}

void V8Instance::createContexts() {
	mContext = v8::Context::New(mIsolate, nullptr, mGlobalScope->getTemplate());

	mPersContext.Reset(mIsolate, mContext);
	mGlobalContext = std::unique_ptr<v8::Context::Scope>(new v8::Context::Scope(mIsolate, mPersContext));
}

v8::Handle<v8::Script> V8Instance::compileScript(const std::wstring& code) {
	return v8::Script::Compile(v8::String::New((const uint16_t*) code.c_str(), code.length()), v8::String::Empty(), v8::String::Empty());
}

void V8Instance::runScriptGlobal(const std::wstring& code) {
	v8::HandleScope handleScope(mIsolate);

	auto script = compileScript(code);

	script->Run();
}

void V8Instance::runScriptIsolated(const std::wstring& code) {
	v8::HandleScope handleScope(mIsolate);

	v8::Handle<v8::Context> ctx = v8::Context::New(mIsolate, nullptr);
	v8::Context::Scope scope(ctx);

	auto script = compileScript(code);

	script->Run();
}