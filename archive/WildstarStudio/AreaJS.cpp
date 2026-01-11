#include "stdafx.h"
#include "AreaJS.h"
#include "Class.h"

AreaJS::AreaJS(FilePtr file) {
	mFile = std::make_shared<AreaFile>(file->getFile()->getFullPath(), file->getFile());
}

bool AreaJS::load() {
	auto res = mFile->load();
	if (res) {
		auto& chunks = mFile->getChunks();
		for (auto& chunk : chunks) {
			mChunks.push_back(std::make_shared<AreaChunkJS>(chunk));
		}
	}

	return res;
}

uint32 AreaJS::getNumChunks() {
	return mChunks.size();
}

AreaChunkJSPtr AreaJS::getChunk(uint32 index) {
	if (index >= mChunks.size()) {
		return nullptr;
	}

	return mChunks[index];
}

AreaChunkJS::AreaChunkJS(AreaChunkRenderPtr chunk) : mChunk(chunk) {

}

std::wstring AreaChunkJS::getTexture(uint32 index) {
	if (index >= mChunk->getNumTextures()) {
		v8::ThrowException(v8::Exception::Error(v8::String::New("Texture index out of bounds.")));
		return L"";
	}

	auto tex = mChunk->getTexture(index);
	if (tex == nullptr) {
		return L"";
	}

	auto file = tex->getFile();
	if (file == nullptr) {
		return L"";
	}

	return file->getFullPath();
}

void AreaJS::onRegister(Scope& scope) {
#define CNK_FUN(funPtr) [](AreaChunkJSPtr chunk) { return (chunk->getChunk().get()->*funPtr)(); }

	scope
	[
		Class<AreaChunkJS>(L"AreaChunk")
			.function<bool>(L"hasHeightmap", CNK_FUN(&AreaChunkRender::hasHeightmap))
			.function<bool>(L"hasTextures", CNK_FUN(&AreaChunkRender::hasTextureIds))
			.function<bool>(L"hasBlendValues", CNK_FUN(&AreaChunkRender::hasBlendValues))
			.function<bool>(L"hasColorMap", CNK_FUN(&AreaChunkRender::hasColorMap))
			.function<bool>(L"hasUnk1", CNK_FUN(&AreaChunkRender::hasUnk1))
			.function<bool>(L"hasShadowMap", CNK_FUN(&AreaChunkRender::hasShadowMap))
			.function<bool>(L"hasUnk2", CNK_FUN(&AreaChunkRender::hasUnk2))
			.function<bool>(L"hasUnk3", CNK_FUN(&AreaChunkRender::hasUnk3))
			.function<bool>(L"hasUnk4", CNK_FUN(&AreaChunkRender::hasUnk4))
			.function<bool>(L"hasUnk5", CNK_FUN(&AreaChunkRender::hasUnk5))
			.function<bool>(L"hasUnk6", CNK_FUN(&AreaChunkRender::hasUnk6))
			.function<bool>(L"hasUnk7", CNK_FUN(&AreaChunkRender::hasUnk7))
			.function<bool>(L"hasUnk8", CNK_FUN(&AreaChunkRender::hasUnk8))
			.function<std::vector<uint8>>(L"getData", CNK_FUN(&AreaChunkRender::getData))
			.function<float>(L"getMaxHeight", CNK_FUN(&AreaChunkRender::getMaxHeight))
			.function<float>(L"getAverageHeight", CNK_FUN(&AreaChunkRender::getAverageHeight))
			.function<uint32>(L"getDataFlags", CNK_FUN(&AreaChunkRender::getFlags))
			.function<uint32>(L"getNumTextures", CNK_FUN(&AreaChunkRender::getNumTextures))
			.function<std::vector<uint8>>(L"getColorMap", CNK_FUN(&AreaChunkRender::getColorMap))
			.function<std::vector<uint8>>(L"getBlendMap", CNK_FUN(&AreaChunkRender::getBlendData))
			.function(L"getUnk7", &AreaChunkJS::getUnk7)
			.function(L"getTextureId", &AreaChunkJS::getTextureId)
			.function(L"getTexture", &AreaChunkJS::getTexture),

		Class<AreaJS>(L"Area")
			.constructor<FilePtr>()
			.function(L"load", &AreaJS::load)
			.function(L"getNumChunks", &AreaJS::getNumChunks)
			.function(L"getChunk", &AreaJS::getChunk)
	];
}