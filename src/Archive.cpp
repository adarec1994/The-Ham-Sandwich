#include "Archive.h"
#include <sstream>
#include <filesystem>
#include <iostream>
#include <zlib.h>
#include <lzma.h> // Make sure to include this
#include <cstring>

std::wstring toUnicode(const char* src) {
    std::string s(src);
    return std::wstring(s.begin(), s.end());
}

Archive::Archive(const std::wstring& file) : mPath(file) {
    mIndexFile.open(file, std::ios::binary);
    if (mIndexFile.is_open() == false) {
       throw std::invalid_argument("Unable to open the index file");
    }

    std::filesystem::path p(file);
    p.replace_extension(".archive");

    mPackFile.open(p.wstring(), std::ios::binary);
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
    auto parent = mParent.lock();
    if (parent != nullptr) {
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
    }

    for (auto dirEnt : dirEntries) {
       dirEnt->parseChildren();
    }
}

void Archive::getFileData(FileEntryPtr file, std::vector<uint8>& content)
{
    std::cout << "\n=== getFileData ===\n";

    for (auto& entry : mAarcTable)
    {
        if (memcmp(entry.shaHash, file->getHash(), 20) != 0)
            continue;

        auto& block = mPkDirectoryHeaders[entry.blockIndex];

        std::cout << "Found entry\n";
        std::cout << "Block offset : " << block.directoryOffset << "\n";
        std::cout << "Block size   : " << block.blockSize << "\n";
        std::cout << "Flags        : " << file->getFlags() << "\n";
        std::cout << "Uncomp size  : " << file->getSizeUncompressed() << "\n";

        pkSeek(block.directoryOffset);

        std::vector<uint8> compressed((size_t)block.blockSize);
        pkRead(compressed.data(), (uint32)block.blockSize);

        std::cout << "Read compressed bytes: " << compressed.size() << "\n";

        std::cout << "Header bytes: ";
        for (int i = 0; i < 16 && i < (int)compressed.size(); ++i)
            std::cout << std::hex << (int)compressed[i] << " ";
        std::cout << std::dec << "\n";

        uint32 flags = file->getFlags();

        // RAW / uncompressed
        if (flags != 3 && !(compressed.size() > 5 && compressed[0] == 0x5D))
        {
            std::cout << "Using RAW copy\n";
            content = compressed;
            return;
        }

        // ZLIB
        if (flags == 3)
        {
            uint32 outSize = (uint32)file->getSizeUncompressed();
            std::cout << "Using ZLIB, expected size: " << outSize << "\n";

            content.resize(outSize);

            z_stream zs{};
            zs.next_in = compressed.data();
            zs.avail_in = (uInt)compressed.size();
            zs.next_out = content.data();
            zs.avail_out = (uInt)content.size();

            if (inflateInit(&zs) != Z_OK)
                throw std::runtime_error("zlib init failed");

            int ret = inflate(&zs, Z_FINISH);
            inflateEnd(&zs);

            std::cout << "ZLIB ret=" << ret << " written=" << zs.total_out << "\n";

            if (ret != Z_STREAM_END)
                throw std::runtime_error("zlib inflate failed");

            content.resize(zs.total_out);
            return;
        }

        // RAW LZMA (WildStar) : [5-byte props][raw LZMA stream...]
        if (compressed.size() > 5 && compressed[0] == 0x5D)
        {
            uint32 outSize = (uint32)file->getSizeUncompressed();
            std::cout << "Using RAW LZMA, expected size: " << outSize << "\n";

            content.resize(outSize);

            lzma_filter filters[2]{};
            filters[0].id = LZMA_FILTER_LZMA1;
            filters[1].id = LZMA_VLI_UNKNOWN;

            lzma_ret ret = lzma_properties_decode(&filters[0], nullptr, compressed.data(), 5);
            std::cout << "lzma_properties_decode = " << ret << "\n";
            if (ret != LZMA_OK)
                throw std::runtime_error("properties decode failed");

            lzma_stream strm = LZMA_STREAM_INIT;
            ret = lzma_raw_decoder(&strm, filters);
            std::cout << "lzma_raw_decoder = " << ret << "\n";
            if (ret != LZMA_OK)
                throw std::runtime_error("raw decoder init failed");

            strm.next_in = compressed.data() + 5;
            strm.avail_in = compressed.size() - 5;
            strm.next_out = content.data();
            strm.avail_out = content.size();

            size_t totalWritten = 0;
            lzma_ret step = LZMA_OK;

            while (true)
            {
                step = lzma_code(&strm, LZMA_RUN);
                totalWritten = (size_t)outSize - (size_t)strm.avail_out;

                if (step == LZMA_STREAM_END)
                    break;

                if (step != LZMA_OK)
                {
                    lzma_end(&strm);
                    std::cout << "lzma_code error = " << step << "\n";
                    throw std::runtime_error("raw lzma decode failed");
                }

                if (strm.avail_out == 0)
                    break;
            }

            lzma_end(&strm);

            std::cout << "lzma_code final=" << step
                      << " written=" << totalWritten
                      << " remaining_out=" << strm.avail_out
                      << " remaining_in=" << strm.avail_in
                      << "\n";

            if (totalWritten != outSize)
                std::cout << "Warning: expected " << outSize << " got " << totalWritten << "\n";

            return;
        }
    }

    throw std::runtime_error("File not found in archive");
}
