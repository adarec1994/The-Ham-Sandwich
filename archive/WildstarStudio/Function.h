#pragma once

#include "ScopeObject.h"
#include "FunctionInstance.h"

class Function : public ScopeObject
{
	std::wstring mName;
	FunctionInstance* mInstance;

	template<typename T, typename... Args>
	void expand(T fun, Args... funs) {
		overload(fun);
		expand<Args...>(funs...);
	}

	template<typename T>
	void expand(T fun) {
		overload(fun);
	}

public:
	Function(const std::wstring& name);
	template<typename... Args>
	Function(const std::wstring& name, Args... funs) : Function(name) {
		expand<Args...>(funs...);
	}

	~Function();
	Function(const Function& other);

	template<typename Res, typename... Args>
	Function& overload(std::function<Res (Args...)> fun) {
		mInstance->addOverload(fun);
		return *this;
	}

	void onRegister(v8::Handle<v8::ObjectTemplate> templ);
};