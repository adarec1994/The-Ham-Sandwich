#include "stdafx.h"
#include "BinaryStream.h"
#include "Object.h"
#include "Array.h"

BinaryStream::BinaryStream(std::vector<uint8> data) {
	mStream = data;
}

void BinaryStream::setValue(ObjectPtr obj, const std::wstring& name, const std::wstring& type) {
	auto tl = String::toLower(type);
	if (type == L"u32" || type == L"uint32") {
		obj->set(name, readUInt32());
	} else if (type == L"i32" || type == L"int32") {
		obj->set(name, readInt32());
	} else if (type == L"f" || type == L"float") {
		obj->set(name, readFloat());
	} else if (type == L"d" || type == L"double") {
		obj->set(name, readDouble());
	}
}


ObjectPtr BinaryStream::read(ArrayPtr scheme) {

	ObjectPtr ret = std::make_shared<Object>();
	if (scheme->length() % 2) {
		return ret;
	}

	for (uint32 i = 0; i < scheme->length(); i += 2) {
		auto name = scheme->get<std::wstring>(i);
		auto type = scheme->get<std::wstring>(i + 1);
		setValue(ret, name, type);
	}

	return ret;
}

std::vector<uint8> BinaryStream::readBytes(uint32 numBytes) {
	std::vector<uint8> ret(numBytes);
	memcpy(ret.data(), &mStream[mCurPos], numBytes);
	mCurPos += numBytes;
	return ret;
}

void BinaryStream::seek(int32 mod) {
	mCurPos += mod;
}

void BinaryStream::seekAbs(uint32 mod) {
	mCurPos = mod;
}

uint64 BinaryStream::tell() {
	return mCurPos;
}

uint32 BinaryStream::readUInt32() {
	return _read<uint32>();
}

uint16 BinaryStream::readUInt16() {
	return _read<uint16>();
}

int32 BinaryStream::readInt32() {
	return _read<int32>();
}

float BinaryStream::readFloat() {
	return _read<float>();
}

double BinaryStream::readDouble() {
	return _read<double>();
}