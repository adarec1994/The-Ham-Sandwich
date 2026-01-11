#pragma once

class BinStream
{
	const std::vector<uint8>& mData;
	uint32 mCurPos = 0;

public:
	BinStream(const std::vector<uint8>& data) : mData(data) {

	}

	void seek(uint64 pos) {
		mCurPos = (uint32)pos;
	}

	void seekMod(int32 mod) {
		mCurPos += mod;
	}

	uint32 tell() const { return mCurPos; }

	template<typename T>
	T read() {
		T ret;
		memcpy(&ret, &mData[mCurPos], sizeof(T));
		mCurPos += sizeof(T);
		return ret;
	}

	void read(void* buffer, uint64 numBytes) {
		memcpy(buffer, &mData[mCurPos], (uint32)numBytes);
		mCurPos += (uint32)numBytes;
	}

	const void* getPointer(uint64 offset) const {
		return &mData[(size_t) offset];
	}
};