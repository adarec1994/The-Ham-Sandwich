#include "stdafx.h"
#include "Function.h"

Function::Function(const std::wstring& name) {
	mName = name;

	mInstance = new FunctionInstance();
}

Function::~Function() {

}

Function::Function(const Function& ) {

}

void Function::onRegister(v8::Handle<v8::ObjectTemplate> templ) {
	templ->Set(v8::String::New((const uint16_t*) mName.c_str(), mName.length()), mInstance->getTemplate());
}