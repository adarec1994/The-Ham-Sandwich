#include "stdafx.h"
#include "Texture.h"
#include "IOManager.h"
#include "UIManager.h"

#undef min
#undef max

Texture::TextureFormatEntry Texture::gFormatEntries[27] = {
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, GL_BGRA, 0, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, GL_RGBA, 0, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, GL_R, 4, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 8, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 4, 3, 2, 4, 3, 2, 1, 0, 0, 8, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 1, true },
	{ 4, 3, 2, 4, 3, 2, 1, 0, 0, 16, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 2, true },
	{ 4, 3, 2, 4, 3, 2, 1, 0, 0, 16, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 3, true },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 8, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 16, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 2, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
	{ 1, 0, 0, 1, 0, 0, 1, 0, 0, 4, 0, -1, false },
};

Texture::Texture(FileEntryPtr file) {
	mFile = file;
	mTexture = 0;
}

Texture::Texture() {
	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::loadTexture() {
	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	std::vector<uint8> data;
	sIOMgr->getArchive()->getFileData(mFile, data);

	BinStream strm(data);

	mHeader = strm.read<TexHeader>();
	auto& fmt = gFormatEntries[mHeader.texFormatIndex];
	if (mHeader.signature != 0x00474658 || fmt.format == -1) {
		return;
	}

	for (int32 i = mHeader.mipCount - 1; i >= 0; --i) {
		uint32 curw = mHeader.width >> i;
		uint32 curh = mHeader.height >> i;

		curw = std::max(1u, curw);
		curh = std::max(1u, curh);

		uint32 size = ((curw + 3) / 4) * ((curh + 3) / 4) * fmt.blockSize;
		if (fmt.compressed == false) {
			size = curw * curh * fmt.blockSize;
		}

		std::vector<uint8> textureData(size);
		strm.read(textureData.data(), textureData.size());

		if (fmt.compressed == true) {
			glCompressedTexImage2D(GL_TEXTURE_2D, i, fmt.v11, curw, curh, 0, size, textureData.data());
		} else {
			glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, curw, curh, 0, fmt.v11, GL_UNSIGNED_BYTE, textureData.data());
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;

	UINT  num = 0;
	UINT  size = 0;

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	pImageCodecInfo = (ImageCodecInfo*) (malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}

	free(pImageCodecInfo);
	return -1;
}

void Texture::exportAsPng() {
	std::vector<uint8> imageData(mHeader.width * mHeader.height * 4);

	glBindTexture(GL_TEXTURE_2D, mTexture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, imageData.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	std::async(std::launch::async, [this, imageData]() {
		std::wstringstream strm;
		strm << sIOMgr->getExtractionPath() << mFile->getFullPath() << L".bmp";

		std::wstring dir = std::tr2::sys::path(strm.str()).parent_path();

		SHCreateDirectoryEx(nullptr, dir.c_str(), nullptr);

		Gdiplus::Bitmap bmp(mHeader.width, mHeader.height);
		Gdiplus::BitmapData bmpData;
		bmp.LockBits(&Gdiplus::Rect(0, 0, mHeader.width, mHeader.height), 0, PixelFormat32bppARGB, &bmpData);
		memcpy(bmpData.Scan0, imageData.data(), imageData.size());
		bmp.UnlockBits(&bmpData);

		CLSID idPng;
		GetEncoderClsid(L"image/bmp", &idPng);
		bmp.Save(strm.str().c_str(), &idPng);

		sUIMgr->asyncExtractComplete();
	});
}

std::shared_ptr<Gdiplus::Bitmap> Texture::getBitmap() {
	std::vector<uint32> clrData;
	loadMemory(clrData);

	auto bmp = std::make_shared<Gdiplus::Bitmap>(mHeader.width, mHeader.height, PixelFormat32bppARGB);
	Gdiplus::BitmapData data;
	bmp->LockBits(&Gdiplus::Rect(0, 0, mHeader.width, mHeader.height), 0, PixelFormat32bppARGB, &data);
	memcpy(data.Scan0, clrData.data(), clrData.size() * 4);
	bmp->UnlockBits(&data);

	return bmp;
}

void Texture::loadMemory(std::vector<uint32>& outData) {
	std::vector<uint8> data;
	sIOMgr->getArchive()->getFileData(mFile, data);

	mStream = std::make_shared<BinStream>(data);

	mHeader = mStream->read<TexHeader>();

	outData.resize(mHeader.width * mHeader.height);

	if (mHeader.texFormatIndex >= 27) {
		return;
	}

	auto& fmt = gFormatEntries[mHeader.texFormatIndex];
	if (fmt.format == -1) {
		return;
	}

	for (int32 i = mHeader.mipCount - 1; i >= 1; --i) {
		uint32 curw = mHeader.width >> i;
		uint32 curh = mHeader.height >> i;

		curw = std::max(1u, curw);
		curh = std::max(1u, curh);

		uint32 size = 0;
		if (fmt.v11 == GL_RGBA || fmt.v11 == GL_RGB || fmt.v11 == GL_BGRA || fmt.v11 == GL_R) {
			size = curw * curh * fmt.blockSize;
		} else {
			size = ((curw + 3) / 4) * ((curh + 3) / 4) * fmt.blockSize;
		}

		mStream->seekMod(size);
	}

	switch (fmt.v11) {
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		loadDXT1(outData);
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		loadDXT3(outData);
		break;

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		loadDXT5(outData);
		break;

	case GL_RGB:
	case GL_RGBA:
	case GL_R:
	case GL_BGRA:
		mStream->read(outData.data(), outData.size());
		break;

	default:
		return;
	}
}

void Texture::loadDXT1(std::vector<uint32>& outData) {
	int numBlocksFull = (mHeader.width * mHeader.height) / 16;
	bool partialBlock = ((mHeader.width * mHeader.height) % 16) != 0;

	std::vector<uint32> blockData((numBlocksFull + (partialBlock ? 1 : 0)) * 16);

	for (int32 i = 0; i < numBlocksFull; ++i) {
		dxt1GetBlock( blockData.data() + i * 16);
	}

	if (partialBlock) {
		dxt1GetBlock(blockData.data() + numBlocksFull * 16);
	}

	for (uint32 y = 0; y < mHeader.height; ++y) {
		for (uint32 x = 0; x < mHeader.width; ++x) {
			int bx = x / 4;
			int by = y / 4;

			int ibx = x % 4;
			int iby = y % 4;

			int blockIndex = by * (mHeader.width / 4) + bx;
			int innerIndex = iby * 4 + ibx;
			outData[y * mHeader.width + x] = blockData[blockIndex * 16 + innerIndex];
		}
	}
}

void Texture::loadDXT3(std::vector<uint32>& outData) {
	int numBlocksFull = (mHeader.width * mHeader.height) / 16;
	bool partialBlock = ((mHeader.width * mHeader.height) % 16) != 0;

	std::vector<uint32> blockData((numBlocksFull + (partialBlock ? 1 : 0)) * 16);

	for (int32 i = 0; i < numBlocksFull; ++i) {
		dxt3GetBlock(blockData.data() + i * 16);
	}

	if (partialBlock) {
		dxt3GetBlock(blockData.data() + numBlocksFull * 16);
	}

	for (uint32 y = 0; y < mHeader.height; ++y) {
		for (uint32 x = 0; x < mHeader.width; ++x) {
			int bx = x / 4;
			int by = y / 4;

			int ibx = x % 4;
			int iby = y % 4;

			int blockIndex = by * (mHeader.width / 4) + bx;
			int innerIndex = iby * 4 + ibx;
			outData[y * mHeader.width + x] = blockData[blockIndex * 16 + innerIndex];
		}
	}
}

void Texture::loadDXT5(std::vector<uint32>& outData) {
	int numBlocksFull = (mHeader.width * mHeader.height) / 16;
	bool partialBlock = ((mHeader.width * mHeader.height) % 16) != 0;

	std::vector<uint32> blockData((numBlocksFull + (partialBlock ? 1 : 0)) * 16);

	for (int32 i = 0; i < numBlocksFull; ++i) {
		dxt5GetBlock(blockData.data() + i * 16);
	}

	if (partialBlock) {
		dxt5GetBlock(blockData.data() + numBlocksFull * 16);
	}

	for (uint32 y = 0; y < mHeader.height; ++y) {
		for (uint32 x = 0; x < mHeader.width; ++x) {
			int bx = x / 4;
			int by = y / 4;

			int ibx = x % 4;
			int iby = y % 4;

			int blockIndex = by * (mHeader.width / 4) + bx;
			int innerIndex = iby * 4 + ibx;
			outData[y * mHeader.width + x] = blockData[blockIndex * 16 + innerIndex];
		}
	}
}

void Texture::dxt1GetBlock(uint32* colorPtr) {
	__declspec(thread) static uint8 rgbTmpArray[4][4] = {
		0, 0, 0, 255,
		0, 0, 0, 255,
		0, 0, 0, 255,
		0, 0, 0, 255
	};

	__declspec(thread) static uint16 tableIndices[16] = { 0 };

	uint16 color1 = mStream->read<uint16>();
	uint16 color2 = mStream->read<uint16>();

	RGB565ToRGB8Array(color1, rgbTmpArray[0]);
	RGB565ToRGB8Array(color2, rgbTmpArray[1]);

	uint8* clr1 = rgbTmpArray[0];
	uint8* clr2 = rgbTmpArray[1];
	uint8* clr3 = rgbTmpArray[2];
	uint8* clr4 = rgbTmpArray[3];

	if (color1 > color2) {
		for (int i = 0; i < 3; ++i) {
			clr4[i] = (clr1[i] + 2 * clr2[i]) / 3;
			clr3[i] = (2 * clr1[i] + clr2[i]) / 3;
		}
	} else {
		for (int i = 0; i < 3; ++i) {
			clr3[i] = (clr1[i] + clr2[i]) / 2;
			clr4[i] = 0;
		}
	}

	uint32 indices = mStream->read<uint32>();
	for (uint32 i = 0; i < 16; ++i) {
		tableIndices[i] = ((indices >> (2 * i)) & 3);
	}

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int index = y * 4 + x;
			colorPtr[index] = *(uint32*) (rgbTmpArray[tableIndices[index]]);
		}
	}
}

void Texture::dxt3GetBlock(uint32* colorPtr) {
	__declspec(thread) static uint32 alphaValues[16] = { 0 };
	__declspec(thread) static uint8 rgbTmpArray[4][4] = { { 0, 0, 0, 255 }, { 0, 0, 0, 255 }, { 0, 0, 0, 255 }, { 0, 0, 0, 255 } };
	__declspec(thread) static uint16 tableIndices[16] = { 0 };

	uint64 alpha = mStream->read<uint64>();
	for (int i = 0; i < 16; ++i) {
		alphaValues[i] = (uint8) ((((alpha >> (4 * i)) & 0x0F) / 15.0f) * 255.0f);
	}

	uint16 color1 = mStream->read<uint16>();
	uint16 color2 = mStream->read<uint16>();

	RGB565ToRGB8Array(color1, rgbTmpArray[0]);
	RGB565ToRGB8Array(color2, rgbTmpArray[1]);

	uint8* clr1 = rgbTmpArray[0];
	uint8* clr2 = rgbTmpArray[1];
	uint8* clr3 = rgbTmpArray[2];
	uint8* clr4 = rgbTmpArray[3];

	if (color1 > color2) {
		for (int i = 0; i < 3; ++i) {
			clr4[i] = (clr1[i] + 2 * clr2[i]) / 3;
			clr3[i] = (2 * clr1[i] + clr2[i]) / 3;
		}
	} else {
		for (int i = 0; i < 3; ++i) {
			clr3[i] = (clr1[i] + clr2[i]) / 2;
			clr4[i] = 0;
		}
	}

	uint32 indices = mStream->read<uint32>();
	for (uint32 i = 0; i < 16; ++i) {
		tableIndices[i] = ((indices >> (2 * i)) & 3);
	}

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int index = y * 4 + x;
			uint32 color = *(uint32*) (rgbTmpArray[tableIndices[index]]);
			uint32 alpha = alphaValues[index];
			color &= 0x00FFFFFF;
			color |= (alpha << 24);
			colorPtr[index] = color;
		}
	}
}

void Texture::dxt5GetBlock(uint32* colorPtr) {
	__declspec(thread) static uint8 alphaValues[8] = { 0 };
	__declspec(thread) static uint32 alphaLookup[16] = { 0 };
	__declspec(thread) static uint8 rgbTmpArray[4][4] = { { 0, 0, 0, 255 }, { 0, 0, 0, 255 }, { 0, 0, 0, 255 }, { 0, 0, 0, 255 } };
	__declspec(thread) static uint16 tableIndices[16] = { 0 };

	uint8 alpha1 = mStream->read<uint8>();
	uint8 alpha2 = mStream->read<uint8>();
	alphaValues[0] = alpha1;
	alphaValues[1] = alpha2;

	if (alpha1 > alpha2) {
		for (int i = 0; i < 6; ++i) {
			alphaValues[i + 2] = (uint8) (((6.0f - i) * alpha1 + (1.0f + i) * alpha2) / 7.0f);
		}
	} else {
		for (int i = 0; i < 4; ++i) {
			alphaValues[i + 2] = (uint8) (((4.0f - i) * alpha1 + (1.0f + i) * alpha2) / 5.0f);
		}

		alphaValues[5] = 0;
		alphaValues[6] = 255;
	}

	uint64 lookupValue = mStream->read<uint32>();
	lookupValue |= ((uint64) mStream->read<uint16>()) << 32;

	for (int i = 0; i < 16; ++i) {
		alphaLookup[i] = (lookupValue >> (i * 3)) & 7;
	}

	uint16 color1 = mStream->read<uint16>();
	uint16 color2 = mStream->read<uint16>();

	RGB565ToRGB8Array(color1, rgbTmpArray[0]);
	RGB565ToRGB8Array(color2, rgbTmpArray[1]);

	uint8* clr1 = rgbTmpArray[0];
	uint8* clr2 = rgbTmpArray[1];
	uint8* clr3 = rgbTmpArray[2];
	uint8* clr4 = rgbTmpArray[3];

	if (color1 > color2) {
		for (int i = 0; i < 3; ++i) {
			clr4[i] = (clr1[i] + 2 * clr2[i]) / 3;
			clr3[i] = (2 * clr1[i] + clr2[i]) / 3;
		}
	} else {
		for (int i = 0; i < 3; ++i) {
			clr3[i] = (clr1[i] + clr2[i]) / 2;
			clr4[i] = 0;
		}
	}

	uint32 indices = mStream->read<uint32>();
	for (uint32 i = 0; i < 16; ++i) {
		tableIndices[i] = ((indices >> (2 * i)) & 3);
	}

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int index = y * 4 + x;
			uint32 color = *(uint32*) (rgbTmpArray[tableIndices[index]]);
			uint32 alpha = alphaValues[alphaLookup[index]];
			color &= 0x00FFFFFF;
			color |= (alpha << 24);
			colorPtr[index] = color;
		}
	}
}

void Texture::setRepeat() {
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::setAndGenMipmap() {
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

TexturePtr Texture::fromMemory(uint32 w, uint32 h, const std::vector<uint32>& colors) {
	TexturePtr tex = std::shared_ptr<Texture>(new Texture());
	glBindTexture(GL_TEXTURE_2D, tex->getId());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

void Texture::saveToFile(const std::wstring& file) {
	glBindTexture(GL_TEXTURE_2D, mTexture);
	GLint width = 0, height = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	if (width == 0 || height == 0) {
		return;
	}

	std::vector<uint32> colors(width * height);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, colors.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	static ULONG_PTR gdiToken = 0xFFFFFFFF;
	if (gdiToken == 0xFFFFFFFF) {
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartup(&gdiToken, &input, nullptr);
	}

	Gdiplus::Bitmap bmp(width, height, PixelFormat32bppARGB);
	Gdiplus::BitmapData data;
	bmp.LockBits(&Gdiplus::Rect(0, 0, width, height), 0, PixelFormat32bppARGB, &data);
	memcpy(data.Scan0, colors.data(), colors.size() * 4);
	bmp.UnlockBits(&data);

	CLSID encoder;
	if (GetEncoderClsid(L"image/png", &encoder) < 0) {
		return;
	}

	bmp.Save(file.c_str(), &encoder);
}