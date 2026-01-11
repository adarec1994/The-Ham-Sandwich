#pragma once

#include "Object.h"
#include "ClassInstance.h"

template<typename T>
static typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	if (value->IsInt32() || value->IsUint32() || value->IsNumber()) {
		return (T) value->ToInteger()->IntegerValue();
	} else {
		throw std::exception("Invalid type of object, cannot convert to integer");
	}
}

template<typename T>
static typename std::enable_if<std::is_same<T, bool>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	if (value->IsInt32() || value->IsUint32() || value->IsNumber()) {
		int64 val = value->IntegerValue();
		return val != 0;
	} else if (value->IsBoolean()) {
		return value->BooleanValue();
	} else {
		throw std::exception("Invalid type of object, cannot convert to boolean");
	}
}

template<typename T>
static typename std::enable_if<std::is_same<T, std::wstring>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	auto str = value->ToString();
	std::wstring ret;
	std::vector<uint16_t> buffer(str->Length());
	str->Write(buffer.data(), 0, buffer.size());
	ret.assign((const wchar_t*) buffer.data(), buffer.size());
	return ret;
}

template<typename T>
static typename std::enable_if<std::is_floating_point<T>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	if (value->IsNumber() == false) {
		throw std::exception("Invalid type of object, cannot convert to float");
	}

	return (T) value->ToNumber()->NumberValue();
}

template<typename T>
static typename std::enable_if<is_shared_ptr<T>::value && !std::is_same<T, ObjectPtr>::value && !std::is_same<T, ArrayPtr>::value && !std::is_same<T, ValuePtr>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	if (value->IsObject() == false) {
		throw std::exception("Value is not an object");
	}

	auto obj = value->ToObject();
	if (obj->InternalFieldCount() < 3) {
		throw std::exception("Ivanlid object");
	}

	auto hash = obj->GetInternalField(0)->ToUint32()->Value();
	if (hash != typeid(typename remove_shared<T>::type).hash_code()) {
		GenericClassInstance* gci = (GenericClassInstance*) obj->GetInternalField(2).As<v8::External>()->Value();
		auto res = gci->upcast<typename remove_shared<T>::type>(obj);
		if (res != nullptr) {
			return res;
		}

		throw std::exception("Object does not match expected type");
	}

	auto ext = obj->GetInternalField(1).As<v8::External>();
	T* ptr = (T*) ext->Value();
	return *ptr;
}

/*template<typename T>
static typename std::enable_if<std::is_same<T, ObjectPtr>::value, T>::type unwrap(const v8::Handle<v8::Value>& value) {
	if (value->IsObject() == false) {
		throw std::exception("Value is not an object");
	}

	return std::make_shared<Object>(value);
}*/

template<typename T>
static typename std::enable_if<std::is_same<T, ObjectPtr>::value, v8::Handle<v8::Value>>::type ObjectWrap::wrap(T value) {
	return value->getHandle();
}

template<typename T>
static typename std::enable_if<std::is_same<T, ArrayPtr>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	if (value->IsArray() == false) {
		throw std::exception("Value is not an array");
	}

	return std::make_shared<Array>(value->ToObject());
}

template<typename T>
static typename std::enable_if<std::is_same<T, ValuePtr>::value, T>::type ObjectWrap::unwrap(const v8::Handle<v8::Value>& value) {
	return std::make_shared<Value>(value);
}

template<typename T>
static typename std::enable_if<is_shared_ptr<T>::value && !std::is_same<T, ObjectPtr>::value && !std::is_same<T, ArrayPtr>::value && !std::is_same<T, ValuePtr>::value, v8::Handle<v8::Value>>::type ObjectWrap::wrap(T value) {
	if (value == nullptr) {
		return v8::Null();
	}
	
	auto cls = GenericClassInstance::getTypeInstance<typename remove_shared<T>::type>();
	if (cls == nullptr) {
		throw std::exception("Type is not registered!");
	}

	return cls->createInstance(SharedPtrWrap<typename remove_shared<T>::type>(value));
}