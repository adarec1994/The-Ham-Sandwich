#pragma once
#include <vector>
#include <string>
#include <memory>
#include <list>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <regex>
#include <sstream>
#include <unordered_map>

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint8_t uint8;
typedef int64_t int64;
typedef int32_t int32;

#pragma pack(push, 1)

struct PackDirectoryHeader
{
    uint64 directoryOffset;
    uint64 blockSize;
};

struct AIDX
{
    uint32 magic;
    uint32 version;
    uint32 unk1;
    uint32 rootBlock;
};

struct AARCEntry
{
    uint32 blockIndex;
    uint8 shaHash[20];
    uint64 uncompressedSize;
};

#pragma pack(pop)

class Archive;
typedef std::shared_ptr<Archive> ArchivePtr;

class ArchiveFile;
typedef std::shared_ptr<ArchiveFile> ArchiveFilePtr;

class IArchiveEntry;
class IArchiveFileEntry;
class DirectoryEntry;
class FileEntry;
class IndexFile;

typedef std::shared_ptr<IArchiveEntry> IArchiveEntryPtr;
typedef std::shared_ptr<IArchiveFileEntry> IArchiveFileEntryPtr;
typedef std::shared_ptr<DirectoryEntry> DirectoryEntryPtr;
typedef std::shared_ptr<FileEntry> FileEntryPtr;
typedef std::shared_ptr<IndexFile> IndexFilePtr;

typedef IArchiveEntry IFileSystemEntry;
typedef IArchiveEntryPtr IFileSystemEntryPtr;

class IArchiveEntry : public std::enable_shared_from_this<IArchiveEntry>
{
protected:
    std::weak_ptr<IArchiveEntry> mParent;
    std::list<IArchiveEntryPtr> mChildren;
    std::wstring mEntryName;

public:
    virtual ~IArchiveEntry() = default;

    std::shared_ptr<IArchiveEntry> getParent() const { return mParent.lock(); }

    std::string getFileName() const {
        return std::string(mEntryName.begin(), mEntryName.end());
    }

    std::wstring getEntryName() const { return mEntryName; }

    const std::list<IArchiveEntryPtr>& getChildren() const { return mChildren; }

    virtual bool isDirectory() const = 0;
    virtual bool hasChildren() const { return false; }

    std::wstring getFullPath() const {
        std::wstring path = mEntryName;
        auto parent = mParent.lock();
        while (parent != nullptr) {
            if (!parent->getEntryName().empty()) {
                path = parent->getEntryName() + L"\\" + path;
            }
            parent = parent->getParent();
        }
        return path;
    }

    std::string getFullPathNarrow() const {
        std::string path = getFileName();
        auto parent = mParent.lock();
        while (parent != nullptr) {
            if (!parent->getFileName().empty()) {
                path = parent->getFileName() + "/" + path;
            }
            parent = parent->getParent();
        }
        return path;
    }
};

class IArchiveFileEntry : public IArchiveEntry
{
public:
    virtual ~IArchiveFileEntry() = default;

    bool isDirectory() const override { return false; }

    virtual const uint8* getHash() const = 0;
    virtual uint32 getFlags() const = 0;
    virtual uint64 getUncompressedSize() const = 0;
    virtual uint64 getCompressedSize() const = 0;
};

class DirectoryEntry : public IArchiveEntry
{
protected:
    ArchivePtr mArchive;
    uint32 mNextBlock;
    bool mIsRootDirectory = false;
    bool mChildrenParsed = false;

public:
    DirectoryEntry(IArchiveEntryPtr parent, const std::wstring& name, uint32 nextBlock, ArchivePtr archive);
    virtual ~DirectoryEntry() = default;

    void parseChildren();
    uint32 countChildren();

    bool isDirectory() const override { return true; }
    bool hasChildren() const override { return !mChildren.empty(); }

    void setRoot(bool root) { mIsRootDirectory = root; }

    void getFiles(std::vector<IArchiveFileEntryPtr>& files);
    void getEntries(std::list<IArchiveEntryPtr>& entries, bool recursive = false);
    IArchiveEntryPtr findEntry(const std::wstring& path);
    IArchiveEntryPtr findEntry(const std::string& path);
    IArchiveFileEntryPtr findFileEntry(const std::wstring& path);
    IArchiveFileEntryPtr findFileEntry(const std::string& path);
    std::vector<IArchiveFileEntryPtr> getFiles(const std::string& pattern);
    std::vector<IArchiveFileEntryPtr> getFiles(const std::wstring& pattern);
};

class FileEntry : public IArchiveFileEntry
{
    uint32 mFlags = 0;
    uint64 mUnkValue = 0;
    uint64 mCompressedSize = 0;
    uint64 mUncompressedSize = 0;
    uint8 mHash[20] = { 0 };

public:
    FileEntry(IArchiveEntryPtr parent, const std::wstring& name, const std::vector<uint8>& data);
    virtual ~FileEntry() = default;

    const uint8* getHash() const override { return mHash; }
    uint32 getFlags() const override { return mFlags; }
    uint64 getUncompressedSize() const override { return mUncompressedSize; }
    uint64 getCompressedSize() const override { return mCompressedSize; }
};

class IndexFile
{
    ArchivePtr mArchive;
    DirectoryEntryPtr mRoot;

public:
    IndexFile(ArchivePtr archive, DirectoryEntryPtr root);

    IArchiveEntryPtr findEntry(const std::string& path);
    IArchiveEntryPtr findEntry(const std::wstring& path);
    std::vector<IArchiveFileEntryPtr> getFiles(const std::string& pattern);
    std::vector<IArchiveFileEntryPtr> getFiles(const std::wstring& pattern);
    DirectoryEntryPtr getRoot() const { return mRoot; }
};

class ArchiveFile : public std::enable_shared_from_this<ArchiveFile>
{
    std::filesystem::path mPath;
    std::ifstream mFile;
    std::vector<PackDirectoryHeader> mDirectoryHeaders;
    std::vector<AARCEntry> mAarcTable;
    uint32 mDirCount = 0;
    uint64 mDirStart = 0;

public:
    ArchiveFile(const std::filesystem::path& archivePath);

    static ArchiveFilePtr fromFile(const std::filesystem::path& path);

    void load();

    bool hasEntry(const uint8* hash) const;
    bool getFileData(const uint8* hash, uint64 uncompressedSize, uint32 flags, std::vector<uint8>& content);

    std::ifstream& getFile() { return mFile; }
    const std::vector<PackDirectoryHeader>& getDirectoryHeaders() const { return mDirectoryHeaders; }
    const std::vector<AARCEntry>& getAarcTable() const { return mAarcTable; }
};

class Archive : public std::enable_shared_from_this<Archive>
{
    std::filesystem::path mIndexPath;
    std::ifstream mIndexFile;
    std::ifstream mPackFile;

    uint32 mDirectoryCount = 0;
    uint64 mDirectoryTableStart = 0;
    AIDX mIndexHeader;
    std::vector<PackDirectoryHeader> mDirectoryHeaders;
    DirectoryEntryPtr mFileRoot;
    uint32 mFileCount = 0;

    std::vector<PackDirectoryHeader> mPkDirectoryHeaders;
    std::vector<AARCEntry> mAarcTable;
    uint32 mPkDirCount = 0;
    uint64 mPkDirStart = 0;

    ArchiveFilePtr mCoreData;
    IndexFilePtr mIndexFileObj;

    std::unordered_map<std::string, FileEntryPtr> mFileCache;
    bool mCacheBuilt = false;

    void loadIndexTree();
    void buildFileCache();
    void buildFileCacheRecursive(const IArchiveEntryPtr& entry, const std::string& currentPath);

public:
    Archive(const std::filesystem::path& indexPath, ArchiveFilePtr coreData = nullptr);

    static ArchivePtr fromFile(const std::filesystem::path& indexPath, ArchiveFilePtr coreData = nullptr);

    void loadIndexInfo();
    void loadArchiveInfo();
    void asyncLoad();

    IndexFilePtr getIndexFile() const { return mIndexFileObj; }

    bool openFileStream(IArchiveFileEntryPtr entry, std::vector<uint8>& content);
    bool openFileStream(FileEntryPtr entry, std::vector<uint8>& content);

    bool getFileData(IArchiveFileEntryPtr entry, std::vector<uint8>& content) { return openFileStream(entry, content); }
    bool getFileData(FileEntryPtr entry, std::vector<uint8>& content) { return openFileStream(entry, content); }

    IArchiveFileEntryPtr getFileInfoByPath(const std::string& path);
    IArchiveFileEntryPtr getFileInfoByPath(const std::wstring& path);

    IArchiveEntryPtr getByPath(const std::wstring& path);
    IArchiveEntryPtr getByPath(const std::string& path);

    FileEntryPtr findFileCached(const std::string& path);
    FileEntryPtr findFileByNameCached(const std::string& filename);

    DirectoryEntryPtr getRoot() const { return mFileRoot; }
    std::filesystem::path getPath() const { return mIndexPath; }
    uint32 getFileCount() const { return mFileCount; }

    const std::vector<PackDirectoryHeader>& getBlockTable() const { return mDirectoryHeaders; }
    const std::vector<AARCEntry>& getAarcTable() const { return mAarcTable; }
    const std::vector<PackDirectoryHeader>& getPkDirectoryHeaders() const { return mPkDirectoryHeaders; }

    template<typename T>
    T idxRead() {
        T ret;
        if (mIndexFile.read(reinterpret_cast<char*>(&ret), sizeof(T))) {
            return ret;
        }
        throw std::runtime_error("Reached end of index file!");
    }

    void idxRead(void* data, uint32 numBytes) {
        if (!mIndexFile.read((char*)data, numBytes)) {
            throw std::runtime_error("Reached end of index file!");
        }
    }

    void idxSeek(uint64 pos) {
        if (!mIndexFile.seekg(pos, std::ios::beg)) {
            throw std::invalid_argument("Unable to seek in index file.");
        }
    }

    void idxSeekMod(int64 mod) {
        auto current = static_cast<int64>(mIndexFile.tellg());
        if (mod < 0 && static_cast<uint64>(std::abs(mod)) > static_cast<uint64>(current)) {
            throw std::invalid_argument("Unable to seek before file start");
        }
        idxSeek(static_cast<uint64>(current + mod));
    }

    uint64 idxTell() {
        return static_cast<uint64>(mIndexFile.tellg());
    }

    template<typename T>
    T pkRead() {
        T ret;
        if (mPackFile.read(reinterpret_cast<char*>(&ret), sizeof(T))) {
            return ret;
        }
        throw std::runtime_error("Reached end of archive file!");
    }

    void pkRead(void* data, uint32 numBytes) {
        if (!mPackFile.read((char*)data, numBytes)) {
            throw std::runtime_error("Reached end of archive file!");
        }
    }

    void pkSeek(uint64 pos) {
        if (!mPackFile.seekg(pos, std::ios::beg)) {
            throw std::invalid_argument("Unable to seek in archive file.");
        }
    }

    uint64 pkTell() {
        return static_cast<uint64>(mPackFile.tellg());
    }

    static const uint32 FileMagic = 0x5041434B;
    static const uint32 IndexMagic = 0x41494458;
    static const uint32 FileMagicArchive = 0x5041434B;
    static const uint32 AarcMagic = 0x41415243;
};

std::string patternToRegex(const std::string& pattern);
std::string normalizePath(const std::string& path);
bool pathMatchesPattern(const std::string& path, const std::string& pattern);