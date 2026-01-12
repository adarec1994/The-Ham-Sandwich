#include "stdafx.h"
#include "Files.h"
#include "Class.h"
#include "Namespace.h"
#include "Function.h"
#include "IOManager.h"
#include "Object.h"
#include "BinStream.h"
#include "Array.h"
#include "HexEditor.h"
#include "BinaryStream.h"
#include "Value.h"
#include "Compression.h"
#include "Enum.h"

File::File(std::wstring path) {
	if (sIOMgr->getArchive() == nullptr) {
		v8::ThrowException(v8::String::New("InvalidOperation: Cannot create files before archive is loaded."));
		return;
	}

	auto entry = sIOMgr->getArchive()->getByPath(path);
	if (entry == nullptr) {
		std::wstringstream strm;
		strm << L"FileNotFound: Could not find file '" << path << L"'.";

		v8::ThrowException(v8::String::New((const uint16_t*) strm.str().c_str(), strm.str().length()));
		return;
	}

	if (entry->isDirectory()) {
		std::wstringstream strm;
		strm << L"InvalidOperation: File '" << path << L"' is a directory. File class only supports files.";

		v8::ThrowException(v8::String::New((const uint16_t*) strm.str().c_str(), strm.str().length()));
		return;
	}

	mFile = std::dynamic_pointer_cast<FileEntry>(entry);
}



void File::assertStream() {
	if (mStream != nullptr) {
		return;
	}

	sIOMgr->getArchive()->getFileData(mFile, mContent);
	mStream = std::unique_ptr<BinStream>(new BinStream(mContent));
}

void File::setValue(ObjectPtr obj, const std::wstring& name, const std::wstring& type) {
	auto tl = String::toLower(type);
	if (type == L"u32" || type == L"uint32") {
		obj->set(name, readUInt32());
	} else if (type == L"i32" || type == L"int32") {
		obj->set(name, readInt32());
	} else if (type == L"f" || type == L"float") {
		obj->set(name, readFloat());
	} else if (type == L"d" || type == L"double") {
		obj->set(name, readDouble());
	}
}

FileEntryPtr File::getFile() {
	return mFile;
}

uint64 File::getFileSize() {
	return mFile->getSizeUncompressed();
}

std::wstring File::getFileName() {
	return mFile->getFullPath();
}

ObjectPtr File::read(ArrayPtr scheme) {
	assertStream();

	ObjectPtr ret = std::make_shared<Object>();
	if (scheme->length() % 2) {
		return ret;
	}

	for (uint32 i = 0; i < scheme->length(); i += 2) {
		auto name = scheme->get<std::wstring>(i);
		auto type = scheme->get<std::wstring>(i + 1);
		setValue(ret, name, type);
	}

	return ret;
}

std::vector<uint8> File::readBytes(uint32 numBytes) {
	std::vector<uint8> ret(numBytes);
	assertStream();
	mStream->read(ret.data(), numBytes);
	return ret;
}

void File::seek(int32 mod) {
	assertStream();
	mStream->seekMod(mod);
}

uint64 File::tell() {
	return mStream->tell();
}

uint32 File::readUInt32() {
	assertStream();
	return mStream->read<uint32>();
}

int32 File::readInt32() {
	assertStream();
	return mStream->read<int32>();
}

float File::readFloat() {
	assertStream();
	return mStream->read<float>();
}

double File::readDouble() {
	assertStream();
	return mStream->read<double>();
}

DiscFile::DiscFile(std::wstring fileName, bool binary) {
	mFile.open(fileName, std::ios::binary);
}

DiscFile::~DiscFile() {
	if (mFile.is_open()) {
		mFile.close();
	}
}

void DiscFile::close() {
	mFile.close();
}

void DiscFile::write(ValuePtr value) {
	if (mIsBinary == false) {
		if (value->is<int64>()) {
			mFile << value->as<int64>();
		} else if (value->is<double>()) {
			mFile << value->as<double>();
		} else if (value->is<std::wstring>()) {
			mFile << value->as<std::string>();
		} else {
			v8::ThrowException(v8::String::New("TypeError: Only integrals, numbers and strings can be written to a file"));
		}
	} else {
		v8::ThrowException(v8::String::New("TypeError: File is binary. Use writeBinary instead"));
	}
}

void DiscFile::writeLine(ValuePtr value) {
	write(value);
	mFile << std::endl;
}

void DiscFile::writeBinary(BinaryStreamPtr strm) {
	mFile.write((const char*) strm->getPtr(), strm->size());
}

void Image::writeBitmap(std::wstring file, uint32 width, uint32 height, uint32 fmt, BinaryStreamPtr data) {
	Gdiplus::PixelFormat pixFmt;
	switch (fmt) {
	case RGB565:
		{
			data->seekAbs(0);
			std::vector<uint8> dataConv(width * height * 4);
			pixFmt = PixelFormat32bppARGB;
			for (uint32 i = 0; i < width * height; ++i) {
				uint16 value = data->readUInt16();
				uint32 r = value & 0x1F;
				uint32 g = (value >> 5) & 0x3F;
				uint32 b = (value >> 11) & 0x1F;
				uint32 a = 0xFF;
				r = (uint32) ((r / 31.0f) * 255.0f);
				g = (uint32) ((g / 63.0f) * 255.0f);
				b = (uint32) ((b / 31.0f) * 255.0f);
				dataConv[i * 4 + 0] = r;
				dataConv[i * 4 + 1] = g;
				dataConv[i * 4 + 2] = b;
				dataConv[i * 4 + 3] = a;
			}

			data = std::make_shared<BinaryStream>(dataConv);
			break;
		}

	case ARGB32:
		pixFmt = PixelFormat32bppARGB;
		break;

	case ARGB16:
		{
			data->seekAbs(0);
			std::vector<uint8> dataConv(width * height * 4);
			pixFmt = PixelFormat32bppARGB;
			for (uint32 i = 0; i < width * height; ++i) {
				uint16 value = data->readUInt16();
				uint32 r = value & 0xF;
				uint32 g = (value >> 4) & 0xF;
				uint32 b = (value >> 8) & 0xF;
				uint32 a = (value >> 12) & 0xF;
				r = (uint32) ((r / 15.0f) * 255.0f);
				g = (uint32) ((g / 15.0f) * 255.0f);
				b = (uint32) ((b / 15.0f) * 255.0f);
				a = (uint32) ((a / 15.0f) * 255.0f);
				dataConv[i * 4 + 0] = r;
				dataConv[i * 4 + 1] = g;
				dataConv[i * 4 + 2] = b;
				dataConv[i * 4 + 3] = a;
			}

			data = std::make_shared<BinaryStream>(dataConv);
		}
		break;

	default:
		v8::ThrowException(v8::Exception::Error(v8::String::New("Invalid pixel format. Supported: RGB565, ARGB32")));
		return;
	}

	data->seekAbs(0);

	Gdiplus::Bitmap bmp(width, height, pixFmt);
	Gdiplus::BitmapData bmpData;
	bmp.LockBits(&Gdiplus::Rect(0, 0, width, height), 0, pixFmt, &bmpData);
	memcpy(bmpData.Scan0, data->getPtr(), data->size());
	bmp.UnlockBits(&bmpData);

	CLSID encoder;
	if (GetEncoderClsid(L"image/png", &encoder) < 0) {
		v8::ThrowException(v8::Exception::Error(v8::String::New("Unable to get encoder for png.")));
		return;
	}

	bmp.Save(file.c_str(), &encoder);
}

void FileLibrary::onRegister(Scope& scope) {
	scope
	[
		Namespace(L"IO")
		[
			Class<File>(L"File")
				.constructor<std::wstring>()
				.property(L"fileSize", &File::getFileSize)
				.property(L"fileName", &File::getFileName)
				.function(L"readUInt32", &File::readUInt32)
				.function(L"readInt32", &File::readInt32)
				.function(L"readFloat", &File::readFloat)
				.function(L"readDouble", &File::readDouble)
				.function(L"read", &File::read)
				.function(L"readBytes", &File::readBytes)
				.function(L"seek", &File::seek)
				.function(L"tell", &File::tell),

			Class<BinaryStream>(L"BinaryStream")
				.constructor<std::vector<uint8>>()
				.function(L"readUInt32", &BinaryStream::readUInt32)
				.function(L"readInt32", &BinaryStream::readInt32)
				.function(L"readFloat", &BinaryStream::readFloat)
				.function(L"readDouble", &BinaryStream::readDouble)
				.function(L"read", &BinaryStream::read)
				.function(L"readBytes", &BinaryStream::readBytes)
				.function(L"seek", &BinaryStream::seek)
				.function(L"tell", &BinaryStream::tell),

			Class<ZLib>(L"ZLib")
				.static_function(L"inflate", &ZLib::inflate),

			Class<DiscFile>(L"DiscFile")
				.constructor<std::wstring>()
				.constructor<std::wstring, bool>()
				.function(L"write", &DiscFile::write)
				.function(L"writeLine", &DiscFile::writeLine)
				.function(L"writeBinary", &DiscFile::writeBinary)
				.function(L"close", &DiscFile::close),

			Class<HexEditor>(L"HexEditor")
				.constructor<FilePtr>()
				.function(L"show", &HexEditor::show),

			Enum(L"PixelFormat")
				.value(L"RGB565", Image::RGB565)
				.value(L"ARGB32", Image::ARGB32)
				.value(L"ARGB16", Image::ARGB16),

			Class<Image>(L"Image")
				.static_function(L"write", &Image::writeBitmap)
		]
	];
}