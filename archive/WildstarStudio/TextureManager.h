#pragma once

#include "Texture.h"

SHARED_FWD(TextureManager);

class TextureManager
{
private:
	static TextureManagerPtr gInstance;

	TexturePtr mDefaultTexture;

	std::map<uint32, std::weak_ptr<Texture>> mTextures;
	std::mutex mTexLock;

public:
	void init();

	TexturePtr getTexture(const std::wstring& file);

	static TextureManagerPtr getInstance() {
		if (gInstance == nullptr) {
			gInstance = std::make_shared<TextureManager>();
		}

		return gInstance;
	}
};

#define sTexMgr (TextureManager::getInstance())