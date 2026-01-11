#pragma once

#include "ScopeObject.h"
#include "ObjectWrap.h"

class Enum : public ScopeObject
{
	std::wstring mName;
	v8::Handle<v8::ObjectTemplate> mTemplate;
public:
	Enum(const std::wstring& name);

	template<typename T>
	Enum& value(const std::wstring& name, const T& value) {
		mTemplate->Set(v8::String::New((const uint16_t*) name.c_str(), name.length()), ObjectWrap::wrap(value));
		return *this;
	}

	void onRegister(v8::Handle<v8::ObjectTemplate> templ);
};

#include "ObjectWrap.inl"