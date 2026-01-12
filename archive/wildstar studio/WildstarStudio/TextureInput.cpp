#include "stdafx.h"
#include "TextureInput.h"
#include "GlExt.h"

void TextureInput::attachToProgram(ProgramPtr program) {
	mDestProgram = program;
}

void TextureInput::setTexture(const std::wstring& samplerName, TexturePtr texture) {
	if (mDestProgram == nullptr)
		throw std::runtime_error("mDestProgram == nullptr");

	uint32 index = mDestProgram->getUniformIndex(samplerName);
	
	setTexture(index, texture);
}

void TextureInput::setTexture(uint32 index, TexturePtr texture) {
	if (texture == nullptr) {
		auto itr = mTextureMap.find(index);
		if (itr != mTextureMap.end()) {
			mTextureMap.erase(itr);
		}

		return;
	}

	mTextureMap[index] = texture;
}

void TextureInput::apply() {
	uint32 index = 0;
	for (auto& pair : mTextureMap) {
		glActiveTexture(GL_TEXTURE0 + index);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, pair.second->getId());
		mDestProgram->set(pair.first, index);
		++index;
	}
}

void TextureInput::remove() {
	uint32 index = 0;
	for (; index < mTextureMap.size(); ++index) {
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	glActiveTexture(GL_TEXTURE0);
}