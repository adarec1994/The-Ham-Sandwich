#pragma once

#include "ObjectWrap.h"

class Object
{
	v8::Handle<v8::Object> mObject;
public:
	Object(v8::Handle<v8::Object> obj);
	Object();

	v8::Handle<v8::Object> getHandle() const { return mObject; }

	template<typename T>
	T get(const std::wstring& name);

	template<typename T>
	void set(const std::wstring& name, const T& value) {
		mObject->Set(v8::String::New((const uint16_t*) name.c_str(), name.length()), ObjectWrap::wrap<T>(value));
	}

	void enumProperties(std::list<std::wstring>& props) {
		auto names = mObject->GetPropertyNames();
		for (uint32 i = 0; i < names->Length(); ++i) {
			auto prop = names->Get(i)->ToString();
			std::vector<wchar_t> pname(prop->Length() + 1);
			prop->Write((uint16_t*) pname.data(), 0, pname.size() - 1);
			props.push_back(pname.data());
		}
	}
};

SHARED_TYPE(Object);

#include "ObjectWrap.inl"

template<typename T>
T Object::get(const std::wstring& name)  {
	return ObjectWrap::unwrap<T>(mObject->Get(v8::String::New((const uint16_t*) name.c_str(), name.length())));
}