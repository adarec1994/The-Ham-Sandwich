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

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint8_t uint8;
typedef int64_t int64;

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

class IFileSystemEntry;
typedef std::shared_ptr<IFileSystemEntry> IFileSystemEntryPtr;

class IFileSystemEntry
{
protected:
    std::weak_ptr<IFileSystemEntry> mParent;
    std::list<IFileSystemEntryPtr> mChildren;
    std::wstring mEntryName;

public:
    virtual ~IFileSystemEntry() { }

    std::shared_ptr<IFileSystemEntry> getParent() const { return mParent.lock(); }
    std::wstring getEntryName() const { return mEntryName; }
    const std::list<IFileSystemEntryPtr>& getChildren() const { return mChildren; }

    virtual bool isDirectory() const = 0;
    virtual bool hasChildren() const { return false; }

    std::wstring getFullPath() const {
       auto path = mEntryName;
       auto parent = mParent.lock();
       while (parent != nullptr) {
          if (parent->getEntryName().empty() == false) {
             path = parent->getEntryName() + L"\\" + path;
          }
          parent = parent->getParent();
       }
       return path;
    }

    virtual void dumpFiles(std::wstring basePath, std::wostream& output) = 0;
};

class Archive;
typedef std::shared_ptr<Archive> ArchivePtr;

class DirectoryEntry : public IFileSystemEntry, public std::enable_shared_from_this<DirectoryEntry>
{
protected:
    ArchivePtr mArchive;
    uint32 mNextBlock;
    bool mIsRootDirectory;
public:
    DirectoryEntry(IFileSystemEntryPtr parent, const std::wstring& name, uint32 nextBlock, ArchivePtr archive);

    virtual ~DirectoryEntry() { }

    virtual void parseChildren();
    uint32 countChildren();

    bool hasChildren() const override { return mChildren.size() > 0; }

    void dumpFiles(std::wstring basePath, std::wostream& output) override;
    void setRoot(bool root) { mIsRootDirectory = root; }
    void getFiles(std::vector<IFileSystemEntryPtr>& files);
    void getEntries(std::list<IFileSystemEntryPtr>& entries, bool recursive = false);
    IFileSystemEntryPtr getFile(std::wstring path);

    bool isDirectory() const override { return true; }
};

typedef std::shared_ptr<DirectoryEntry> DirectoryEntryPtr;

class FileEntry : public IFileSystemEntry
{
    uint32 flags = 0;
    uint64 unkValue = 0;
    uint64 compressedSize = 0;
    uint64 uncompressedSize = 0;
    uint8 hash[20] = { 0 };
    uint32 block = 0;
public:
    FileEntry(IFileSystemEntryPtr parent, const std::wstring& name, const std::vector<uint8>& junk) {
       mParent = parent;
       mEntryName = name;
       flags = *(uint32*)(junk.data());
       unkValue = *(uint64*)(junk.data() + 4);
       uncompressedSize = *(uint64*)(junk.data() + 12);
       compressedSize = *(uint64*)(junk.data() + 20);
       memcpy(hash, junk.data() + 28, 20);
    }

    virtual ~FileEntry() { }

    bool isDirectory() const override { return false; }

    const uint8* getHash() const { return hash; }
    uint32 getFlags() const { return flags; }

    uint64 getSizeUncompressed() const { return uncompressedSize; }

    void dumpFiles(std::wstring basePath, std::wostream& output) override
    {
       output << basePath << mEntryName << L" (Type: " << flags << L", Compressed: " << compressedSize << L", Uncompressed: " << uncompressedSize << L")" << std::endl;
    }
};

typedef std::shared_ptr<FileEntry> FileEntryPtr;

class Archive : public std::enable_shared_from_this<Archive>
{
    uint32 mDirectoryCount;
    uint64 mDirectoryTableStart;
    AIDX mIndexHeader;
    std::vector<PackDirectoryHeader> mDirectoryHeaders;
    DirectoryEntryPtr mFileRoot;
    std::ifstream mIndexFile;
    std::ifstream mPackFile;
    uint32 mFileCount;
    std::vector<PackDirectoryHeader> mPkDirectoryHeaders;
    std::vector<AARCEntry> mAarcTable;
    uint32 mPkDirCount;
    uint64 mPkDirStart;
    std::filesystem::path mPath;

    void loadIndexTree();

public:
    Archive(const std::filesystem::path& indexPath);

    void loadIndexInfo();
    void loadArchiveInfo();

    void asyncLoad();

    void getFileData(FileEntryPtr file, std::vector<uint8>& content);

    DirectoryEntryPtr getRoot() const { return mFileRoot; }
    std::filesystem::path getPath() const { return mPath; }

    IFileSystemEntryPtr getByPath(const std::wstring& path) const;

    const std::vector<PackDirectoryHeader>& getBlockTable() const { return mDirectoryHeaders; }
    std::ifstream& getIndexFile() { return mIndexFile; }

    template<typename T>
    T idxRead() {
       T ret;
       if (mIndexFile.read(reinterpret_cast<char*>(&ret), sizeof(T))) {
          return ret;
       }
       throw std::runtime_error("Reached end of file!");
    }

    void idxRead(void* data, uint32 numBytes) {
       if (mIndexFile.read((char*)data, numBytes)) {
          return;
       }
       throw std::runtime_error("Reached end of file!");
    }

    void idxSeek(uint64 pos) {
       if (mIndexFile.seekg(pos, std::ios::beg)) {
          return;
       }
       throw std::invalid_argument("Unable to seek beyond the file.");
    }

    void idxSeekMod(int64 mod) {
       if (abs(mod) > idxTell() && mod < 0) {
          throw std::invalid_argument("Unable to seek before file");
       }
       idxSeek(idxTell() + mod);
    }

    uint32 idxTell() {
       return static_cast<uint32>(mIndexFile.tellg());
    }

    template<typename T>
    T pkRead() {
       T ret;
       if (mPackFile.read(reinterpret_cast<char*>(&ret), sizeof(T))) {
          return ret;
       }
       throw std::runtime_error("Reached end of file!");
    }

    void pkRead(void* data, uint32 numBytes) {
       if (mPackFile.read((char*)data, numBytes)) {
          return;
       }
       throw std::runtime_error("Reached end of file!");
    }

    void pkSeek(uint64 pos) {
       if (mPackFile.seekg(pos, std::ios::beg)) {
          return;
       }
       throw std::invalid_argument("Unable to seek beyond the file.");
    }

    void pkSeekMod(int64 mod) {
       if (abs(mod) > pkTell() && mod < 0) {
          throw std::invalid_argument("Unable to seek before file");
       }
       pkSeek(pkTell() + mod);
    }

    uint32 pkTell() {
       return static_cast<uint32>(mIndexFile.tellg());
    }

    uint32 getFileCount() const { return mFileCount; }

    static const uint32 FileMagic = 0x5041434B;         // 'PACK'
    static const uint32 IndexMagic = 0x41494458;        // 'AIDX'
    static const uint32 FileMagicArchive = 0x5041434B;  // 'PACK'
    static const uint32 AarcMagic = 0x41415243;         // 'AARC'
};