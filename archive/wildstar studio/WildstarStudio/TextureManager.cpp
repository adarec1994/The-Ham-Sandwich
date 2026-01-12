#include "stdafx.h"
#include "TextureManager.h"
#include "IOManager.h"

TextureManagerPtr TextureManager::gInstance;

void TextureManager::init() {
	mDefaultTexture = Texture::fromMemory(2, 2, { 0xFFFFFFFF, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF });
}

TexturePtr TextureManager::getTexture(const std::wstring& file) {
	auto hash = String::hash(String::toLower(file));

	std::lock_guard<std::mutex> l(mTexLock);
	auto itr = mTextures.find(hash);
	if (itr != mTextures.end() && itr->second.expired() == false) {
		return itr->second.lock();
	}

	auto filePtr = sIOMgr->getArchive()->getByPath(file);
	if (filePtr == nullptr || filePtr->isDirectory()) {
		return mDefaultTexture;
	}

	auto fileEntry = std::dynamic_pointer_cast<FileEntry>(filePtr);
	auto tex = std::make_shared<Texture>(fileEntry);
	tex->loadTexture();
	mTextures[hash] = tex;

	return tex;
}