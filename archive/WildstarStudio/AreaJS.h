#pragma once

#include "AreaFile.h"
#include "Files.h"
#include "V8Instance.h"

class AreaChunkJS
{
private:
	AreaChunkRenderPtr mChunk;

public:
	AreaChunkJS(AreaChunkRenderPtr chunk);

	AreaChunkRenderPtr& getChunk() { return mChunk; }
	std::wstring getTexture(uint32 index);

	uint32 getUnk7(uint32 index) {
		try { 
			return mChunk->getUnk7(index); 
		} catch (...) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("Has no unk7"))); 
			return 0;
		}
	}

	uint32 getTextureId(uint32 index) {
		try {
			return mChunk->getTextureId(index);
		} catch (...) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("Has no texture ids")));
			return 0;
		}
	}
};

SHARED_TYPE(AreaChunkJS);

class AreaJS
{
private:
	AreaFilePtr mFile;
	std::vector<AreaChunkJSPtr> mChunks;

public:
	AreaJS(FilePtr file);

	bool load();
	uint32 getNumChunks();
	AreaChunkJSPtr getChunk(uint32 index);

	static void onRegister(Scope& scope);
};