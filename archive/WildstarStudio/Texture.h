#pragma once

#include "Archive.h"

SHARED_FWD(Texture);

class Texture
{
	struct TexHeader
	{
		uint32 signature;
		uint32 version;
		uint32 width;
		uint32 height;
		uint32 depth;
		uint32 sides;
		uint32 mipCount;
		uint32 texFormatIndex;
	};

	struct TextureFormatEntry
	{
		uint32 v1, v2, v3, v4, v5, v6, v7, v8, v9, blockSize, v11, format;
		bool compressed;
	};

	static TextureFormatEntry gFormatEntries[27];

	GLuint mTexture;
	FileEntryPtr mFile;
	TexHeader mHeader;
	std::shared_ptr<BinStream> mStream;

	inline void RGB565ToRGB8Array(uint16 input, uint8* output)
	{
		uint32 r = (uint32) (input & 0x1F);
		uint32 g = (uint32) ((input >> 5) & 0x3F);
		uint32 b = (uint32) ((input >> 11) & 0x1F);

		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);

		output[0] = r;
		output[1] = g;
		output[2] = b;
	}

	void loadMemory(std::vector<uint32>& outData);
	void loadDXT1(std::vector<uint32>& outData);
	void loadDXT3(std::vector<uint32>& outData);
	void loadDXT5(std::vector<uint32>& outData);

	void dxt1GetBlock(uint32* colorPtr);
	void dxt3GetBlock(uint32* colorPtr);
	void dxt5GetBlock(uint32* colorPtr);

	Texture();

public:
	Texture(FileEntryPtr file);

	static TexturePtr fromMemory(uint32 w, uint32 h, const std::vector<uint32>& colors);

	void exportAsPng();

	void saveToFile(const std::wstring& file);
	void setRepeat();
	void setAndGenMipmap();

	void loadTexture();
	GLuint getId() const { return mTexture; }
	FileEntryPtr getFile() const { return mFile; }

	TexHeader getHeader() const { return mHeader; }

	std::shared_ptr<Gdiplus::Bitmap> getBitmap();
};

SHARED_TYPE(Texture);