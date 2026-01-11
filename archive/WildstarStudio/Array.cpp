#include "stdafx.h"
#include "Array.h"

Array::Array(v8::Handle<v8::Object> arr) {
	if (arr->IsArray() == false) {
		throw std::exception("Invalid object type");
	}

	mArray = arr;
}

Array::ArrayValue Array::operator [] (uint32 index) {
	if (index >= length()) {
		throw std::out_of_range("Invalid index into array");
	}

	return ArrayValue(mArray->Get(index));
}