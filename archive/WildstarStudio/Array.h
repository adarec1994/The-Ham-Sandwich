#pragma once

#include "ObjectWrap.h"

class Array
{
	v8::Handle<v8::Object> mArray;
public:
	Array(v8::Handle<v8::Object> arr);

	class ArrayValue
	{
		v8::Handle<v8::Value> mValue;
	public:
		ArrayValue(v8::Handle<v8::Value> val) {
			mValue = val;
		}

		template<typename T>
		operator T() {
			return ObjectWrap::unwrap<T>(mValue);
		}
	};

	uint32 length() const {
		return (uint32) mArray->Get(v8::String::New("length"))->IntegerValue();
	}

	ArrayValue operator [] (uint32 index);

	template<typename T>
	T get(uint32 index);
};

SHARED_TYPE(Array);

#include "ObjectWrap.inl"

template<typename T>
T Array::get(uint32 index) { 
	return ObjectWrap::unwrap<T>(mArray->Get(index)); 
}