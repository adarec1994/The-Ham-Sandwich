#include "Archive.h"
#include <zlib.h>
#include <lzma.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define wcscasecmp _wcsicmp
#else
#include <strings.h>
#include <cwchar>
#define wcscasecmp wcscasecmp
#endif

static std::wstring toWideString(const char* src) {
    if (!src) return L"";
    std::string s(src);
    return std::wstring(s.begin(), s.end());
}

static std::string toNarrowString(const std::wstring& src) {
    return std::string(src.begin(), src.end());
}

static std::wstring normalizePathW(const std::wstring& path) {
    std::wstring result = path;
    std::replace(result.begin(), result.end(), L'\\', L'/');
    return result;
}

static std::string toLowerStr(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string patternToRegex(const std::string& pattern)
{
    std::string regex;
    regex.reserve(pattern.size() * 2);

    for (char c : pattern) {
        switch (c) {
            case '*': regex += ".*"; break;
            case '?': regex += "."; break;
            case '.':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '^':
            case '$':
            case '+':
            case '|':
                regex += '\\';
                regex += c;
                break;
            case '\\': regex += '/'; break;
            default: regex += c; break;
        }
    }

    return regex;
}

std::string normalizePath(const std::string& path)
{
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool pathMatchesPattern(const std::string& path, const std::string& pattern)
{
    std::string normalizedPath = normalizePath(path);
    std::string normalizedPattern = normalizePath(pattern);
    std::string regexPattern = patternToRegex(normalizedPattern);

    try {
        std::regex re(regexPattern, std::regex::icase);
        return std::regex_match(normalizedPath, re);
    } catch (...) {
        return false;
    }
}

FileEntry::FileEntry(IArchiveEntryPtr parent, const std::wstring& name, const std::vector<uint8>& data)
{
    mParent = parent;
    mEntryName = name;

    if (data.size() >= 52) {
        mFlags = *(uint32*)(data.data());
        mUnkValue = *(uint64*)(data.data() + 4);
        mUncompressedSize = *(uint64*)(data.data() + 12);
        mCompressedSize = *(uint64*)(data.data() + 20);
        memcpy(mHash, data.data() + 28, 20);
    }
}

DirectoryEntry::DirectoryEntry(IArchiveEntryPtr parent, const std::wstring& name, uint32 nextBlock, ArchivePtr archive)
    : mArchive(archive), mNextBlock(nextBlock)
{
    mParent = parent;
    mEntryName = name;
}

uint32 DirectoryEntry::countChildren()
{
    auto& blockTable = mArchive->getBlockTable();
    auto nextBlock = blockTable[mNextBlock];

    mArchive->idxSeek(nextBlock.directoryOffset);

    uint32 numDirs = mArchive->idxRead<uint32>();
    uint32 numFiles = mArchive->idxRead<uint32>();

    std::list<DirectoryEntryPtr> dirEnts;

    for (uint32 i = 0; i < numDirs; ++i) {
        uint32 nameOffset = mArchive->idxRead<uint32>();
        uint32 nextBlk = mArchive->idxRead<uint32>();

        auto thisPtr = std::dynamic_pointer_cast<IArchiveEntry>(shared_from_this());
        DirectoryEntryPtr dirEnt = std::make_shared<DirectoryEntry>(thisPtr, L"", nextBlk, mArchive);
        dirEnts.push_back(dirEnt);
    }

    for (auto& dirEnt : dirEnts) {
        numFiles += dirEnt->countChildren();
    }

    return numFiles + numDirs;
}

void DirectoryEntry::parseChildren()
{
    if (mChildrenParsed) return;
    mChildrenParsed = true;

    auto& blockTable = mArchive->getBlockTable();
    auto nextBlock = blockTable[mNextBlock];

    mArchive->idxSeek(nextBlock.directoryOffset);
    uint32 numDirectories = mArchive->idxRead<uint32>();
    uint32 numFiles = mArchive->idxRead<uint32>();
    uint64 curPos = mArchive->idxTell();

    int64 dataSize = numDirectories * 8 + numFiles * 56;
    mArchive->idxSeekMod(dataSize);
    uint64 stringSize = nextBlock.blockSize - 8 - dataSize;
    std::vector<char> nameData((uint32)stringSize);
    mArchive->idxRead(nameData.data(), (uint32)nameData.size());

    mArchive->idxSeek(curPos);
    std::list<DirectoryEntryPtr> dirEntries;

    auto thisPtr = std::dynamic_pointer_cast<IArchiveEntry>(shared_from_this());

    for (uint32 i = 0; i < numDirectories; ++i) {
        uint32 nameOffset = mArchive->idxRead<uint32>();
        uint32 nextBlk = mArchive->idxRead<uint32>();

        std::wstring name = toWideString(nameData.data() + nameOffset);
        DirectoryEntryPtr dirEnt = std::make_shared<DirectoryEntry>(thisPtr, name, nextBlk, mArchive);
        dirEntries.push_back(dirEnt);
        mChildren.push_back(dirEnt);
    }

    for (uint32 i = 0; i < numFiles; ++i) {
        uint32 nameOffset = mArchive->idxRead<uint32>();
        std::vector<uint8> fileData(52);
        mArchive->idxRead(fileData.data(), (uint32)fileData.size());

        std::wstring name = toWideString(nameData.data() + nameOffset);
        FileEntryPtr fe = std::make_shared<FileEntry>(thisPtr, name, fileData);
        mChildren.push_back(fe);
    }

    for (auto& dirEnt : dirEntries) {
        dirEnt->parseChildren();
    }
}

void DirectoryEntry::getFiles(std::vector<IArchiveFileEntryPtr>& files)
{
    parseChildren();

    for (auto& entry : mChildren) {
        if (!entry->isDirectory()) {
            files.push_back(std::dynamic_pointer_cast<IArchiveFileEntry>(entry));
        } else {
            std::dynamic_pointer_cast<DirectoryEntry>(entry)->getFiles(files);
        }
    }
}

void DirectoryEntry::getEntries(std::list<IArchiveEntryPtr>& entries, bool recursive)
{
    parseChildren();

    for (auto& entry : mChildren) {
        entries.push_back(entry);
        if (recursive && entry->isDirectory()) {
            std::dynamic_pointer_cast<DirectoryEntry>(entry)->getEntries(entries, recursive);
        }
    }
}

IArchiveEntryPtr DirectoryEntry::findEntry(const std::wstring& path)
{
    parseChildren();

    std::wstring normalizedPath = normalizePathW(path);

    size_t slashPos = normalizedPath.find(L'/');
    if (slashPos == std::wstring::npos) {
        for (auto& child : mChildren) {
            if (wcscasecmp(child->getEntryName().c_str(), normalizedPath.c_str()) == 0) {
                return child;
            }
        }
        return nullptr;
    }

    std::wstring dir = normalizedPath.substr(0, slashPos);
    std::wstring remain = normalizedPath.substr(slashPos + 1);

    for (auto& child : mChildren) {
        if (child->isDirectory() && wcscasecmp(child->getEntryName().c_str(), dir.c_str()) == 0) {
            return std::dynamic_pointer_cast<DirectoryEntry>(child)->findEntry(remain);
        }
    }

    return nullptr;
}

IArchiveEntryPtr DirectoryEntry::findEntry(const std::string& path)
{
    return findEntry(toWideString(path.c_str()));
}

IArchiveFileEntryPtr DirectoryEntry::findFileEntry(const std::wstring& path)
{
    auto entry = findEntry(path);
    if (entry && !entry->isDirectory()) {
        return std::dynamic_pointer_cast<IArchiveFileEntry>(entry);
    }
    return nullptr;
}

IArchiveFileEntryPtr DirectoryEntry::findFileEntry(const std::string& path)
{
    return findFileEntry(toWideString(path.c_str()));
}

std::vector<IArchiveFileEntryPtr> DirectoryEntry::getFiles(const std::string& pattern)
{
    std::vector<IArchiveFileEntryPtr> allFiles;
    getFiles(allFiles);

    std::vector<IArchiveFileEntryPtr> matches;
    for (auto& file : allFiles) {
        if (pathMatchesPattern(file->getFullPathNarrow(), pattern)) {
            matches.push_back(file);
        }
    }
    return matches;
}

std::vector<IArchiveFileEntryPtr> DirectoryEntry::getFiles(const std::wstring& pattern)
{
    return getFiles(toNarrowString(pattern));
}

IndexFile::IndexFile(ArchivePtr archive, DirectoryEntryPtr root)
    : mArchive(archive), mRoot(root)
{
}

IArchiveEntryPtr IndexFile::findEntry(const std::string& path)
{
    if (!mRoot) return nullptr;
    return mRoot->findEntry(path);
}

IArchiveEntryPtr IndexFile::findEntry(const std::wstring& path)
{
    if (!mRoot) return nullptr;
    return mRoot->findEntry(path);
}

std::vector<IArchiveFileEntryPtr> IndexFile::getFiles(const std::string& pattern)
{
    if (!mRoot) return {};
    return mRoot->getFiles(pattern);
}

std::vector<IArchiveFileEntryPtr> IndexFile::getFiles(const std::wstring& pattern)
{
    if (!mRoot) return {};
    return mRoot->getFiles(pattern);
}

ArchiveFile::ArchiveFile(const std::filesystem::path& archivePath)
    : mPath(archivePath)
{
    mFile.open(archivePath, std::ios::binary);
    if (!mFile.is_open()) {
        throw std::invalid_argument("Unable to open archive file: " + archivePath.string());
    }
}

ArchiveFilePtr ArchiveFile::fromFile(const std::filesystem::path& path)
{
    auto archiveFile = std::make_shared<ArchiveFile>(path);
    archiveFile->load();
    return archiveFile;
}

void ArchiveFile::load()
{
    mFile.seekg(0, std::ios::beg);
    uint32 sig;
    mFile.read(reinterpret_cast<char*>(&sig), sizeof(sig));
    if (sig != Archive::FileMagicArchive) {
        throw std::runtime_error("Invalid archive file format");
    }

    uint32 version;
    mFile.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) {
        throw std::runtime_error("Invalid archive file version");
    }

    uint8 sizeData[548];
    mFile.read(reinterpret_cast<char*>(sizeData), 548);

    mDirCount = *(uint32*)(sizeData + 536);
    mDirStart = *(uint64*)(sizeData + 528);

    mDirectoryHeaders.resize(mDirCount);
    mFile.seekg(mDirStart, std::ios::beg);
    mFile.read(reinterpret_cast<char*>(mDirectoryHeaders.data()), mDirCount * sizeof(PackDirectoryHeader));

    bool hasAarc = false;
    uint32 aarcBlock = 0;
    uint32 aarcEntries = 0;

    for (auto& header : mDirectoryHeaders) {
        mFile.seekg(header.directoryOffset, std::ios::beg);
        if (header.blockSize < 16) continue;

        uint32 signature, ver, craaEntryCount, craaTableBlock;
        mFile.read(reinterpret_cast<char*>(&signature), 4);
        mFile.read(reinterpret_cast<char*>(&ver), 4);
        mFile.read(reinterpret_cast<char*>(&craaEntryCount), 4);
        mFile.read(reinterpret_cast<char*>(&craaTableBlock), 4);

        if (signature == Archive::AarcMagic) {
            hasAarc = true;
            aarcBlock = craaTableBlock;
            aarcEntries = craaEntryCount;
            break;
        }
    }

    if (hasAarc && aarcBlock < mDirectoryHeaders.size()) {
        mFile.seekg(mDirectoryHeaders[aarcBlock].directoryOffset, std::ios::beg);
        mAarcTable.resize(aarcEntries);
        mFile.read(reinterpret_cast<char*>(mAarcTable.data()), aarcEntries * sizeof(AARCEntry));
    }
}

bool ArchiveFile::hasEntry(const uint8* hash) const
{
    for (auto& entry : mAarcTable) {
        if (memcmp(entry.shaHash, hash, 20) == 0) {
            return true;
        }
    }
    return false;
}

bool ArchiveFile::getFileData(const uint8* hash, uint64 uncompressedSize, uint32 flags, std::vector<uint8>& content)
{
    for (auto& entry : mAarcTable) {
        if (memcmp(entry.shaHash, hash, 20) != 0) continue;

        auto& block = mDirectoryHeaders[entry.blockIndex];
        mFile.seekg(block.directoryOffset, std::ios::beg);

        std::vector<uint8> compressed((size_t)block.blockSize);
        mFile.read(reinterpret_cast<char*>(compressed.data()), block.blockSize);

        if (flags != 3 && !(compressed.size() > 5 && compressed[0] == 0x5D)) {
            content = std::move(compressed);
            return true;
        }

        if (flags == 3) {
            uint32 outSize = (uint32)uncompressedSize;
            content.resize(outSize);

            z_stream zs{};
            zs.next_in = compressed.data();
            zs.avail_in = (uInt)compressed.size();
            zs.next_out = content.data();
            zs.avail_out = (uInt)content.size();

            if (inflateInit(&zs) != Z_OK) return false;
            int ret = inflate(&zs, Z_FINISH);
            inflateEnd(&zs);

            if (ret != Z_STREAM_END) return false;
            content.resize(zs.total_out);
            return true;
        }

        if (compressed.size() > 5 && compressed[0] == 0x5D) {
            uint32 outSize = (uint32)uncompressedSize;
            content.resize(outSize);

            lzma_filter filters[2]{};
            filters[0].id = LZMA_FILTER_LZMA1;
            filters[1].id = LZMA_VLI_UNKNOWN;

            lzma_ret ret = lzma_properties_decode(&filters[0], nullptr, compressed.data(), 5);
            if (ret != LZMA_OK) return false;

            lzma_stream strm = LZMA_STREAM_INIT;
            ret = lzma_raw_decoder(&strm, filters);
            if (ret != LZMA_OK) return false;

            strm.next_in = compressed.data() + 5;
            strm.avail_in = compressed.size() - 5;
            strm.next_out = content.data();
            strm.avail_out = content.size();

            while (true) {
                ret = lzma_code(&strm, LZMA_RUN);
                if (ret == LZMA_STREAM_END) break;
                if (ret != LZMA_OK) {
                    lzma_end(&strm);
                    return false;
                }
                if (strm.avail_out == 0) break;
            }

            lzma_end(&strm);
            return true;
        }
    }

    return false;
}

Archive::Archive(const std::filesystem::path& indexPath, ArchiveFilePtr coreData)
    : mIndexPath(indexPath), mCoreData(coreData)
{
    mIndexFile.open(indexPath, std::ios::binary);
    if (!mIndexFile.is_open()) {
        throw std::invalid_argument("Unable to open index file: " + indexPath.string());
    }

    auto archivePath = indexPath;
    archivePath.replace_extension(".archive");
    mPackFile.open(archivePath, std::ios::binary);
    if (!mPackFile.is_open()) {
        throw std::invalid_argument("Unable to open archive file: " + archivePath.string());
    }
}

ArchivePtr Archive::fromFile(const std::filesystem::path& indexPath, ArchiveFilePtr coreData)
{
    auto archive = std::make_shared<Archive>(indexPath, coreData);
    archive->loadIndexInfo();
    archive->loadArchiveInfo();
    archive->mFileRoot->parseChildren();
    archive->buildFileCache();
    archive->mIndexFileObj = std::make_shared<IndexFile>(archive, archive->mFileRoot);
    return archive;
}

void Archive::asyncLoad()
{
    if (mFileRoot) {
        mFileRoot->parseChildren();
    }
    if (!mCacheBuilt) {
        buildFileCache();
    }
    if (!mIndexFileObj) {
        mIndexFileObj = std::make_shared<IndexFile>(shared_from_this(), mFileRoot);
    }
}

void Archive::buildFileCache()
{
    if (mCacheBuilt) return;
    mCacheBuilt = true;

    if (mFileRoot) {
        buildFileCacheRecursive(mFileRoot, "");
    }
}

void Archive::buildFileCacheRecursive(const IArchiveEntryPtr& entry, const std::string& currentPath)
{
    if (!entry) return;

    std::string entryPath = currentPath;
    if (!entry->getFileName().empty()) {
        if (!entryPath.empty()) entryPath += "/";
        entryPath += entry->getFileName();
    }

    if (!entry->isDirectory()) {
        auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry);
        if (fileEntry) {
            std::string lowerPath = toLowerStr(entryPath);
            mFileCache[lowerPath] = fileEntry;
        }
    }

    for (const auto& child : entry->getChildren()) {
        buildFileCacheRecursive(child, entryPath);
    }
}

FileEntryPtr Archive::findFileCached(const std::string& path)
{
    if (!mCacheBuilt) buildFileCache();

    std::string normalized = normalizePath(path);
    std::string lower = toLowerStr(normalized);

    auto it = mFileCache.find(lower);
    if (it != mFileCache.end()) {
        return it->second;
    }
    return nullptr;
}

FileEntryPtr Archive::findFileByNameCached(const std::string& filename)
{
    if (!mCacheBuilt) buildFileCache();

    std::string lowerFilename = toLowerStr(filename);

    for (const auto& [path, entry] : mFileCache) {
        size_t lastSlash = path.rfind('/');
        std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        if (name == lowerFilename) {
            return entry;
        }
    }
    return nullptr;
}

void Archive::loadIndexInfo()
{
    idxSeek(0);
    uint32 sig = idxRead<uint32>();
    if (sig != FileMagic) {
        throw std::runtime_error("Invalid index file format");
    }

    uint32 version = idxRead<uint32>();
    if (version != 1) {
        throw std::runtime_error("Invalid index file version");
    }

    uint8 sizeData[548];
    mIndexFile.read(reinterpret_cast<char*>(sizeData), 548);

    mDirectoryCount = *(uint32*)(sizeData + 536);
    mDirectoryTableStart = *(uint64*)(sizeData + 528);

    mDirectoryHeaders.resize(mDirectoryCount);
    idxSeek(mDirectoryTableStart);
    mIndexFile.read(reinterpret_cast<char*>(mDirectoryHeaders.data()), mDirectoryHeaders.size() * sizeof(PackDirectoryHeader));

    loadIndexTree();
}

void Archive::loadArchiveInfo()
{
    pkSeek(0);

    uint32 sig = pkRead<uint32>();
    if (sig != FileMagicArchive) {
        throw std::runtime_error("Invalid archive file format");
    }

    uint32 version = pkRead<uint32>();
    if (version != 1) {
        throw std::runtime_error("Invalid archive file version");
    }

    uint8 sizeData[548];
    mPackFile.read(reinterpret_cast<char*>(sizeData), 548);

    mPkDirCount = *(uint32*)(sizeData + 536);
    mPkDirStart = *(uint64*)(sizeData + 528);

    mPkDirectoryHeaders.resize(mPkDirCount);
    pkSeek(mPkDirStart);
    pkRead(mPkDirectoryHeaders.data(), mPkDirCount * sizeof(PackDirectoryHeader));

    bool hasAarc = false;
    uint32 aarcBlock = 0;
    uint32 aarcEntries = 0;

    for (auto& header : mPkDirectoryHeaders) {
        pkSeek(header.directoryOffset);
        if (header.blockSize < 16) continue;

        uint32 signature = pkRead<uint32>();
        uint32 ver = pkRead<uint32>();
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
        throw std::runtime_error("Missing AARC table in archive file");
    }

    pkSeek(mPkDirectoryHeaders[aarcBlock].directoryOffset);
    mAarcTable.resize(aarcEntries);
    pkRead(mAarcTable.data(), (uint32)(mAarcTable.size() * sizeof(AARCEntry)));
}

void Archive::loadIndexTree()
{
    bool aidxFound = false;

    for (auto& entry : mDirectoryHeaders) {
        if (entry.blockSize < sizeof(AIDX)) continue;

        idxSeek(entry.directoryOffset);
        AIDX idx = idxRead<AIDX>();
        if (idx.magic == IndexMagic) {
            aidxFound = true;
            mIndexHeader = idx;
            break;
        }
    }

    if (!aidxFound) {
        throw std::runtime_error("Invalid index file format, missing AIDX block");
    }

    mFileRoot = std::make_shared<DirectoryEntry>(nullptr, L"", mIndexHeader.rootBlock, shared_from_this());
    mFileRoot->setRoot(true);
    mFileCount = mFileRoot->countChildren() + 1;
}

IArchiveFileEntryPtr Archive::getFileInfoByPath(const std::string& path)
{
    auto cached = findFileCached(path);
    if (cached) return cached;
    if (!mFileRoot) return nullptr;
    return mFileRoot->findFileEntry(path);
}

IArchiveFileEntryPtr Archive::getFileInfoByPath(const std::wstring& path)
{
    return getFileInfoByPath(toNarrowString(path));
}

IArchiveEntryPtr Archive::getByPath(const std::wstring& path)
{
    if (!mFileRoot) return nullptr;
    return mFileRoot->findEntry(path);
}

IArchiveEntryPtr Archive::getByPath(const std::string& path)
{
    if (!mFileRoot) return nullptr;
    return mFileRoot->findEntry(path);
}

bool Archive::openFileStream(IArchiveFileEntryPtr entry, std::vector<uint8>& content)
{
    auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry);
    if (!fileEntry) return false;
    return openFileStream(fileEntry, content);
}

bool Archive::openFileStream(FileEntryPtr file, std::vector<uint8>& content)
{
    const uint8* hash = file->getHash();
    uint64 uncompressedSize = file->getUncompressedSize();
    uint32 flags = file->getFlags();

    if (mCoreData && mCoreData->hasEntry(hash)) {
        return mCoreData->getFileData(hash, uncompressedSize, flags, content);
    }

    for (auto& entry : mAarcTable) {
        if (memcmp(entry.shaHash, hash, 20) != 0) continue;

        auto& block = mPkDirectoryHeaders[entry.blockIndex];

        pkSeek(block.directoryOffset);

        std::vector<uint8> compressed((size_t)block.blockSize);
        pkRead(compressed.data(), (uint32)block.blockSize);

        if (flags != 3 && !(compressed.size() > 5 && compressed[0] == 0x5D)) {
            content = std::move(compressed);
            return true;
        }

        if (flags == 3) {
            uint32 outSize = (uint32)uncompressedSize;
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

            if (ret != Z_STREAM_END)
                throw std::runtime_error("zlib inflate failed");

            content.resize(zs.total_out);
            return true;
        }

        if (compressed.size() > 5 && compressed[0] == 0x5D) {
            uint32 outSize = (uint32)uncompressedSize;
            content.resize(outSize);

            lzma_filter filters[2]{};
            filters[0].id = LZMA_FILTER_LZMA1;
            filters[1].id = LZMA_VLI_UNKNOWN;

            lzma_ret ret = lzma_properties_decode(&filters[0], nullptr, compressed.data(), 5);
            if (ret != LZMA_OK)
                throw std::runtime_error("LZMA properties decode failed");

            lzma_stream strm = LZMA_STREAM_INIT;
            ret = lzma_raw_decoder(&strm, filters);
            if (ret != LZMA_OK)
                throw std::runtime_error("LZMA raw decoder init failed");

            strm.next_in = compressed.data() + 5;
            strm.avail_in = compressed.size() - 5;
            strm.next_out = content.data();
            strm.avail_out = content.size();

            while (true) {
                ret = lzma_code(&strm, LZMA_RUN);
                if (ret == LZMA_STREAM_END) break;
                if (ret != LZMA_OK) {
                    lzma_end(&strm);
                    throw std::runtime_error("LZMA decode failed");
                }
                if (strm.avail_out == 0) break;
            }

            lzma_end(&strm);
            return true;
        }
    }

    return false;
}