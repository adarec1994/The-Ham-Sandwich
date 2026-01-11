#pragma once

#include "V8Instance.h"

SHARED_FWD(FileEntry);
SHARED_FWD(BinaryStream);

class File
{
	FileEntryPtr mFile;
	std::vector<uint8> mContent;
	std::unique_ptr<BinStream> mStream;

	void assertStream();
	void setValue(ObjectPtr obj, const std::wstring& name, const std::wstring& type);

public:
	File(std::wstring path);

	FileEntryPtr getFile();
	uint64 getFileSize();
	std::wstring getFileName();
	ObjectPtr read(ArrayPtr scheme);
	uint32 readUInt32();
	int32 readInt32();
	float readFloat();
	double readDouble();
	std::vector<uint8> readBytes(uint32 numBytes);

	void seek(int32 mod);
	uint64 tell();

	FileEntryPtr getFile() const { return mFile; }

	std::vector<uint8>& getContent() { assertStream(); return mContent; }
};

class DiscFile
{
	std::ofstream mFile;
	bool mIsBinary;
public:
	DiscFile(std::wstring fileName, bool binary = false);
	~DiscFile();

	void close();

	void write(ValuePtr value);
	void writeLine(ValuePtr value);

	void writeBinary(BinaryStreamPtr stream);
};

class Image
{
public:
	static const int RGB565 = 1;
	static const int ARGB32 = 2;
	static const int ARGB16 = 3;

	static void writeBitmap(std::wstring fileName, uint32 width, uint32 height, uint32 fmt, BinaryStreamPtr bytes);
};

SHARED_TYPE(File);

class FileLibrary
{
public:
	static void onRegister(Scope& scope);
};