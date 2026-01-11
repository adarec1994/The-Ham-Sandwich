#include "stdafx.h"
#include "Enum.h"

Enum::Enum(const std::wstring& name) {
	mTemplate = v8::ObjectTemplate::New();
	mName = name;
}

void Enum::onRegister(v8::Handle<v8::ObjectTemplate> templ) {
	templ->Set(v8::String::New((const uint16_t*) mName.c_str(), mName.length()), mTemplate);
}