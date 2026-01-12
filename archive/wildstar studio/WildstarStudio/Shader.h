#pragma once

#include "IOManager.h"
#include "BinStream.h"

#pragma pack(push, 1)

struct ShaderHeader
{
	uint32 signature;
	uint32 version;
	uint64 numPermutations;
	uint64 ofsPermutations;
	uint64 numUnk1;
	uint64 ofsUnk1;
	uint64 numUnk2;
	uint64 ofsUnk2;
	uint64 numUnk3;
	uint64 ofsUnk3;
	uint64 unk4;
};

struct ShaderPermute
{
	uint64 numBytes;
	uint64 offset;
};

#pragma pack(pop)

class Shader
{
	FileEntryPtr mFile;
	std::shared_ptr<BinStream> mStream;
	std::vector<uint8> mContent;
	ShaderHeader mHeader;

	std::vector<std::wstring> mPermutations;

public:
	Shader(FileEntryPtr file);
	bool load();

	const std::vector<std::wstring>& getPermutations() const { return mPermutations; }
};