#pragma once

#include "BinStream.h"
#include "Object.h"
#include "Array.h"

class BinaryStream
{
	std::vector<uint8> mStream;
	uint32 mCurPos = 0;

	void setValue(ObjectPtr obj, const std::wstring& name, const std::wstring& type);

	template<typename T>
	T _read() {
		T ret;
		memcpy(&ret, &mStream[mCurPos], sizeof(T));
		mCurPos += sizeof(T);
		return ret;
	}

public:
	BinaryStream(std::vector<uint8> data);

	ObjectPtr read(ArrayPtr scheme);
	uint32 readUInt32();
	int32 readInt32();
	uint16 readUInt16();
	float readFloat();
	double readDouble();
	std::vector<uint8> readBytes(uint32 numBytes);

	uint32 size() { return mStream.size(); }
	uint8* getPtr() { return mStream.data(); }

	void seek(int32 mod);
	void seekAbs(uint32 val);
	uint64 tell();
};

SHARED_TYPE(BinaryStream);