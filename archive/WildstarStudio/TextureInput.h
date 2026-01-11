#pragma once

#include "Texture.h"
#include "Program.h"

class TextureInput
{
	std::map<uint32, TexturePtr> mTextureMap;
	ProgramPtr mDestProgram;
public:

	void attachToProgram(ProgramPtr program);

	void setTexture(const std::wstring& samplerName, TexturePtr texture);
	void setTexture(uint32 samplerIndex, TexturePtr texture);
	void apply();
	void remove();
};

typedef std::shared_ptr<TextureInput> TextureInputPtr;