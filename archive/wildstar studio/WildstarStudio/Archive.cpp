#include "stdafx.h"
#include "Archive.h"
#include "IOManager.h"

Archive::Archive(const std::wstring& file) {
	mIndexFile.open(file, std::ios::binary);
	if (mIndexFile.is_open() == false) {
		throw std::invalid_argument("Unable to open the file");
	}

	std::tr2::sys::path p(file);
	std::wstringstream strm;
	strm << p.replace_extension(L"").wstring() << L".archive";

	mPackFile.open(strm.str(), std::ios::binary);
	if (mPackFile.is_open() == false) {
		throw std::invalid_argument("Unable to open archive file");
	}
}

void Archive::loadIndexInfo() {
	idxSeek(0);
	uint32 sig = idxRead<uint32>();
	if (sig != FileMagic) {
		throw std::runtime_error("Invalid file format");
	}

	uint32 version = idxRead<uint32>();
	if (version != 1) {
		throw std::runtime_error("Invalid file format");
	}

	uint8 sizeData[548];
	mIndexFile.read((char*) sizeData, 548);

	mDirectoryCount = *(uint32*) (sizeData + 536);
	mDirectoryTableStart = *(uint64*) (sizeData + 528);

	mDirectoryHeaders.resize(mDirectoryCount);
	idxSeek(mDirectoryTableStart);
	mIndexFile.read((char*) mDirectoryHeaders.data(), mDirectoryHeaders.size() * sizeof(PackDirectoryHeader));

	loadIndexTree();
}

void Archive::loadArchiveInfo() {
	pkSeek(0);

	uint32 sig = pkRead<uint32>();
	if (sig != FileMagicArchive) {
		throw std::runtime_error("Invalid file format");
	}

	uint32 version = pkRead<uint32>();
	if (version != 1) {
		throw std::runtime_error("Invalid file format");
	}

	uint8 sizeData[548];
	mPackFile.read((char*) sizeData, 548);

	mPkDirCount = *(uint32*) (sizeData + 536);
	mPkDirStart = *(uint64*) (sizeData + 528);

	mPkDirectoryHeaders.resize(mPkDirCount);
	pkSeek(mPkDirStart);
	pkRead(mPkDirectoryHeaders.data(), mPkDirCount * sizeof(PackDirectoryHeader));

	bool hasAarc = false;
	uint32 aarcBlock = 0;
	uint32 aarcEntries = 0;

	for (auto header : mPkDirectoryHeaders) {
		pkSeek(header.directoryOffset);
		if (header.blockSize < 16)
			continue;

		uint32 signature = pkRead<uint32>();
		uint32 version = pkRead<uint32>();
		uint32 craaEntryCount = pkRead<uint32>();
		uint32 craaTableBlock = pkRead<uint32>();

		if (signature == AarcMagic) {
			hasAarc = true;
			aarcBlock = craaTableBlock;
			aarcEntries = craaEntryCount;
			break;
		}
	}

	if (!hasAarc) {
		throw std::exception("Missing CRAA table in file");
	}

	pkSeek(mPkDirectoryHeaders[aarcBlock].directoryOffset);
	mAarcTable.resize(aarcEntries);
	pkRead(mAarcTable.data(), mAarcTable.size() * sizeof(AARCEntry));
}

void Archive::loadIndexTree() {
	bool aidxFound = false;

	for (auto& entry : mDirectoryHeaders) {
		if (entry.blockSize < sizeof(AIDX)) {
			continue;
		}

		idxSeek(entry.directoryOffset);
		AIDX idx = idxRead<AIDX>();
		if (idx.magic == IndexMagic) {
			aidxFound = true;
			mIndexHeader = idx;
			break;
		}
	}

	if (aidxFound == false) {
		throw std::runtime_error("Invalid file format, missing AIDX block");
	}

	mFileRoot = std::make_shared<DirectoryEntry>(std::shared_ptr<IFileSystemEntry>(), L"", mIndexHeader.rootBlock, shared_from_this());
	mFileCount = mFileRoot->countChildren();
	mFileCount += 1;
}

void Archive::asyncLoad() {
	mFileRoot->parseChildren();
}

IFileSystemEntryPtr Archive::getByPath(const std::wstring& path) const {
	return mFileRoot->getFile(path);
}

DirectoryEntry::DirectoryEntry(IFileSystemEntryPtr parent, const std::wstring& name, uint32 nextBlock, ArchivePtr archive) {
	mArchive = archive;
	mEntryName = name;
	mNextBlock = nextBlock;
	mParent = parent;
}

uint32 DirectoryEntry::countChildren() {
	auto& blockTable = mArchive->getBlockTable();

	auto nextBlock = blockTable[mNextBlock];

	mArchive->idxSeek(nextBlock.directoryOffset);

	uint32 numDirs = mArchive->idxRead<uint32>();
	uint32 numFiles = mArchive->idxRead<uint32>();

	std::list<DirectoryEntryPtr> dirEnts;

	for (uint32 i = 0; i < numDirs; ++i) {
		uint32 nameOffset = mArchive->idxRead<uint32>();
		uint32 nextBlock = mArchive->idxRead<uint32>();

		DirectoryEntryPtr dirEnt = std::make_shared<DirectoryEntry>(shared_from_this(), L"", nextBlock, mArchive);
		dirEnts.push_back(dirEnt);
	}

	for (auto& dirEnt : dirEnts) {
		numFiles += dirEnt->countChildren();
	}

	return numFiles + numDirs;
}

void DirectoryEntry::getFiles(std::vector<IFileSystemEntryPtr>& files) {
	for (auto entry : mChildren) {
		if (entry->isDirectory() == false) {
			files.push_back(entry);
		} else {
			std::dynamic_pointer_cast<DirectoryEntry>(entry)->getFiles(files);
		}
	}
}

void DirectoryEntry::getEntries(std::list<IFileSystemEntryPtr>& entries, bool recursive) {
	for (auto entry : mChildren) {
		entries.push_back(entry);
		if (recursive && entry->isDirectory() == true) {
			std::dynamic_pointer_cast<DirectoryEntry>(entry)->getEntries(entries, recursive);
		}
	}
}

IFileSystemEntryPtr DirectoryEntry::getFile(std::wstring path) {
	std::wstring::size_type t = path.find(L'\\');
	if (t == std::wstring::npos) {
		for (auto child : mChildren) {
			if (child->getEntryName() == path) {
				return child;
			}
		}

		return nullptr;
	}

	std::wstring dir = path.substr(0, t);
	std::wstring remain = path.substr(t + 1);
	for (auto child : mChildren) {
		if (child->getEntryName() == dir && child->isDirectory())
			return std::dynamic_pointer_cast<DirectoryEntry>(child)->getFile(remain);
	}

	return nullptr;
}

void DirectoryEntry::dumpFiles(std::wstring basePath, std::wostream& strm) {
	std::wstringstream newPath;
	newPath << basePath << mEntryName;
	if (mParent != nullptr) {
		newPath << L"\\";
	}

	for (auto child : mChildren) {
		child->dumpFiles(newPath.str(), strm);
	}
}

void DirectoryEntry::parseChildren() {
	auto& blockTable = mArchive->getBlockTable();

	auto nextBlock = blockTable[mNextBlock];

	mArchive->idxSeek(nextBlock.directoryOffset);
	uint32 numDirectories = mArchive->idxRead<uint32>();
	uint32 numFiles = mArchive->idxRead<uint32>();
	uint64 curPos = mArchive->idxTell();

	int64 dataSize = numDirectories * 8 + numFiles * 56;
	mArchive->idxSeekMod(dataSize);
	uint64 stringSize = nextBlock.blockSize - 8 - dataSize;
	std::vector<char> nameData((uint32) stringSize);
	mArchive->idxRead(nameData.data(), nameData.size());

	mArchive->idxSeek(curPos);
	std::list<DirectoryEntryPtr> dirEntries;

	for (uint32 i = 0; i < numDirectories; ++i) {
		uint32 nameOffset = mArchive->idxRead<uint32>();
		uint32 nextBlock = mArchive->idxRead<uint32>();

		DirectoryEntryPtr dirEnt = std::make_shared<DirectoryEntry>(shared_from_this(), toUnicode(nameData.data() + nameOffset), nextBlock, mArchive);
		dirEntries.push_back(dirEnt);
		mChildren.push_back(dirEnt);
	}

	for (uint32 i = 0; i < numFiles; ++i) {
		uint32 nameOffset = mArchive->idxRead<uint32>();
		std::vector<uint8> junk(52);
		mArchive->idxRead(junk.data(), junk.size());

		FileEntryPtr fe = std::make_shared<FileEntry>(shared_from_this(), toUnicode(nameData.data() + nameOffset), junk);
		mChildren.push_back(fe);
		sIOMgr->asyncFileLoaded();
	}

	for (auto dirEnt : dirEntries) {
		dirEnt->parseChildren();
	}

	sIOMgr->asyncFileLoaded();
}

void Archive::getFileData(FileEntryPtr file, std::vector<uint8>& content) {
	for (auto entry : mAarcTable) {
		if (memcmp(entry.shaHash, file->getHash(), 20) == 0) {
			auto block = mPkDirectoryHeaders[entry.blockIndex];
			pkSeek(block.directoryOffset);
			std::vector<uint8> compressed((uint32) block.blockSize);
			pkRead(compressed.data(), (uint32) block.blockSize);

			if (file->getFlags() == 3) {
				content.resize((uint32) file->getSizeUncompressed());

				z_stream strm = { 0 };
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				strm.avail_in = (uint32) block.blockSize;
				strm.next_in = compressed.data();
				strm.avail_out = (uint32) content.size();
				strm.next_out = content.data();

				inflateInit(&strm);

				auto ret = inflate(&strm, Z_NO_FLUSH);
				if (ret == Z_STREAM_ERROR) {
					throw std::exception();
				}

				uint64 size = content.size() - strm.avail_out;
				content.resize((uint32) size);
				inflateEnd(&strm);

			} else {
				content.assign(compressed.begin(), compressed.end());
			}
			return;
		}
	}
}