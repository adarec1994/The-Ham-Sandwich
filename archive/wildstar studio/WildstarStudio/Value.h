#pragma once

SHARED_FWD(Object);
SHARED_FWD(Array);
SHARED_FWD(TypedArray);

#include "ObjectWrap.h"

class Value
{
private:
	v8::Handle<v8::Value> mValue;

public:
	Value(v8::Handle<v8::Value> val) {
		mValue = val;
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, bool>::type is() const {
		return mValue->IsInt32() || mValue->IsUint32();
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, bool>::type is() const {
		return mValue->IsNumber();
	}

	template<typename T>
	typename std::enable_if<std::is_same<std::wstring, T>::value, bool>::type is() const {
		return mValue->IsString();
	}

	template<typename T>
	typename std::enable_if<std::is_same<std::string, T>::value, bool>::type is() const {
		return mValue->IsString();
	}

	template<typename T>
	typename std::enable_if<std::is_same<ObjectPtr, T>::value, bool>::type is() const {
		return mValue->IsObject();
	}

	template<typename T>
	typename std::enable_if<std::is_same<ArrayPtr, T>::value, bool>::type is() const {
		return mValue->IsArray();
	}

	template<typename T>
	typename std::enable_if<std::is_same<TypedArrayPtr, T>::value, bool>::type is() const {
		return mValue->IsTypedArray();
	}

	template<typename T>
	T as();
};

SHARED_TYPE(Value);

#include "ObjectWrap.inl"

template<typename T>
T Value::as() {
	return ObjectWrap::unwrap<T>(mValue);
}