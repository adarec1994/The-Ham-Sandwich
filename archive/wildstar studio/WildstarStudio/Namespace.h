#pragma once

#include "Scope.h"

class Namespace : public Scope, ScopeObject
{
	std::wstring mName;
public:
	Namespace(const std::wstring& name) : Scope(v8::ObjectTemplate::New()) {
		mName = name;
	}

	void onRegister(v8::Handle<v8::ObjectTemplate> templ) {
		templ->Set(v8::String::New((const uint16_t*) mName.c_str(), mName.length()), mTemplate);
	}

	ScopeObject& operator [] (ScopeObject& obj) {
		obj.onRegister(mTemplate);
		return *this;
	}

	ScopeObject& operator [] (ScopeObjectCollection coll) {
		coll.registerAll(mTemplate);
		return *this;
	}
};