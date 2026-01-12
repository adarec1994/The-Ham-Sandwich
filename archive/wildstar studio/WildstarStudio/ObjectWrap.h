#pragma once

template<typename T>
class is_shared_ptr : public std::false_type
{

};

template<typename T>
class is_shared_ptr<std::shared_ptr<T>> : public std::true_type
{

};

template<typename T>
class remove_shared
{
public:
	typedef typename T type;
};

template<typename T>
class remove_shared<std::shared_ptr<T>>
{
public:
	typedef typename T type;
};

class Object;
typedef std::shared_ptr<Object> ObjectPtr;

class Array;
typedef std::shared_ptr<Array> ArrayPtr;

class Value;
typedef std::shared_ptr<Value> ValuePtr;

class ObjectWrap
{
public:
	template<typename T>
	static typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, bool>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, std::wstring>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, std::string>::value, T>::type unwrap(const v8::Handle<v8::Value>& value) {
		auto str = value->ToString();
		uint32 len = str->Utf8Length();
		std::vector<char> buff(len + 1);
		str->WriteUtf8(buff.data(), len);
		return buff.data();
	}

	template<typename T>
	static typename std::enable_if<std::is_floating_point<T>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<is_shared_ptr<T>::value && !std::is_same<T, ObjectPtr>::value && !std::is_same<T, ArrayPtr>::value && !std::is_same<T, ValuePtr>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, ObjectPtr>::value, T>::type unwrap(const v8::Handle<v8::Value>& value) {
		if (value->IsObject() == false) {
			throw std::exception("Value is not an object");
		}

		return std::make_shared<Object>(value->ToObject());
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, ArrayPtr>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, ValuePtr>::value, T>::type unwrap(const v8::Handle<v8::Value>& value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, std::vector<uint8>>::value, T> ::type unwrap(const v8::Handle<v8::Value>& value) {
		std::vector<uint8> ret(value->ToObject()->GetIndexedPropertiesExternalArrayDataLength());
		memcpy(ret.data(), value->ToObject()->GetIndexedPropertiesExternalArrayData(), ret.size());
		return ret;
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, v8::Handle<v8::Value>>::value, v8::Handle<v8::Value>>::type wrap(const v8::Handle<v8::Value>& value) {
		return value;
	}

	template<typename T>
	static typename std::enable_if<std::is_integral<T>::value, v8::Handle<v8::Value>>::type wrap(T value) {
		return v8::Integer::New(v8::Isolate::GetCurrent(), (int32_t) value);
	}

	template<typename T>
	static typename std::enable_if<std::is_floating_point<T>::value, v8::Handle<v8::Value>>::type wrap(T value) {
		return v8::Number::New(v8::Isolate::GetCurrent(), (double) value);
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, std::wstring>::value, v8::Handle<v8::Value>>::type wrap(T value) {
		return v8::String::New((const uint16_t*) value.data(), value.length());
	}

	template<typename T>
	static typename std::enable_if<std::is_same<T, ObjectPtr>::value, v8::Handle<v8::Value>>::type wrap(T value);

	template<typename T>
	static typename std::enable_if<std::is_same<T, std::vector<uint8>>::value, v8::Handle<v8::Value>> ::type wrap(T value) {
		auto arrBuffer = v8::ArrayBuffer::New(v8::Isolate::GetCurrent(), value.size());
		memcpy(arrBuffer->BaseAddress(), value.data(), value.size());
		auto ui8Array = v8::Uint8Array::New(arrBuffer, 0, value.size());
		return ui8Array;
	}

	template<typename T>
	static typename std::enable_if<is_shared_ptr<T>::value && !std::is_same<T, ObjectPtr>::value && !std::is_same<T, ArrayPtr>::value && !std::is_same<T, ValuePtr>::value, v8::Handle<v8::Value>>::type wrap(T value);
};