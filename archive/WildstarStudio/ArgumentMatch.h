#pragma once

#include "ObjectWrap.h"

template<typename T>
class ClassInstance;

enum class ArgumentMatch
{
	Exact,
	Partial,
	None
};

class Argument
{
public:
	template<typename T>
	static typename std::enable_if<std::is_integral<T>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		if (value->IsInt32() || value->IsUint32()) {
			return ArgumentMatch::Exact;
		} else if (value->IsNumber()) {
			return ArgumentMatch::Partial;
		}

		return ArgumentMatch::None;
	}

	template<typename T>
	static typename std::enable_if<std::is_floating_point<T>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		if (value->IsInt32() || value->IsUint32()) {
			return ArgumentMatch::Partial;
		} else if (value->IsNumber()) {
			return ArgumentMatch::Exact;
		}

		return ArgumentMatch::None;
	}

	template<typename T>
	static typename std::enable_if<std::is_same<std::wstring, T>::value || std::is_same<std::string, T>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		if (value->IsString()) {
			return ArgumentMatch::Exact;
		}

		return ArgumentMatch::Partial;
	}

	template<typename T>
	static typename std::enable_if<is_shared_ptr<T>::value && !std::is_same<T, ArrayPtr>::value && !std::is_same<T, ObjectPtr>::value && !std::is_same<T, ValuePtr>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		if (value->IsObject() == false) {
			return ArgumentMatch::None;
		}

		auto obj = value->ToObject();
		if (obj->InternalFieldCount() < 3) {
			return ArgumentMatch::None;
		}

		if (obj->GetInternalField(2)->IsExternal() == false || obj->GetInternalField(1)->IsExternal() == false) {
			return ArgumentMatch::None;
		}

		auto if1 = obj->GetInternalField(0);
		if (if1->IsInt32() == false && if1->IsUint32() == false) {
			return ArgumentMatch::None;
		}

		auto hash = if1->ToUint32()->Value();
		if (hash != typeid(typename remove_shared<T>::type).hash_code()) {
			GenericClassInstance* gci = (GenericClassInstance*) obj->GetInternalField(2).As<v8::External>()->Value();
			if (gci->canUpcast<typename remove_shared<T>::type>()) {
				return ArgumentMatch::Partial;
			}

			return ArgumentMatch::None;
		}

		return ArgumentMatch::Exact;
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, ObjectPtr>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		return (value->IsObject() ? ArgumentMatch::Exact : ArgumentMatch::None);
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, ArrayPtr>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		return (value->IsArray() ? ArgumentMatch::Exact : ArgumentMatch::None);
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, std::vector<uint8>>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		return value->IsUint8Array() ? ArgumentMatch::Exact : ArgumentMatch::None;
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, ValuePtr>::value, ArgumentMatch>::type match(v8::Handle<v8::Value> value) {
		return ArgumentMatch::Exact;
	}
};