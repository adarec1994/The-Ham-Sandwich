#pragma once
#include <vector>
#include <cstring>
#include <cstdint>
#include <string>

typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef uint16_t uint16;
typedef uint8_t uint8;

class BinStream
{
	const std::vector<uint8>& mData;
	uint32 mCurPos = 0;

public:
	BinStream(const std::vector<uint8>& data) : mData(data) {}

	void seek(uint64 pos) {
		if (pos > mData.size()) mCurPos = (uint32)mData.size();
		else mCurPos = (uint32)pos;
	}

	void seekMod(int32 mod) {
		int64 newPos = mCurPos + mod;
		if (newPos < 0) mCurPos = 0;
		else if (newPos > mData.size()) mCurPos = (uint32)mData.size();
		else mCurPos = (uint32)newPos;
	}

	uint32 tell() const { return mCurPos; }
	uint32 getSize() const { return (uint32)mData.size(); }

	template<typename T>
	T read() {
		if (mCurPos + sizeof(T) > mData.size()) return T();
		T ret;
		memcpy(&ret, &mData[mCurPos], sizeof(T));
		mCurPos += sizeof(T);
		return ret;
	}

	void read(void* buffer, uint64 numBytes) {
		if (mCurPos + numBytes > mData.size()) return;
		memcpy(buffer, &mData[mCurPos], (uint32)numBytes);
		mCurPos += (uint32)numBytes;
	}

	const void* getPointer(uint64 offset) const {
		if (offset >= mData.size()) return nullptr;
		return &mData[(size_t) offset];
	}
};