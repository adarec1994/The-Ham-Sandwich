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

class Archive;
typedef std::shared_ptr<Archive> ArchivePtr;

class ArchiveFile;
typedef std::shared_ptr<ArchiveFile> ArchiveFilePtr;

// Forward declarations
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

// Legacy type aliases for backward compatibility
typedef IArchiveEntry IFileSystemEntry;
typedef IArchiveEntryPtr IFileSystemEntryPtr;

// ============================================================================
// IArchiveEntry - Base interface for archive entries (matches C# interface)
// ============================================================================
class IArchiveEntry : public std::enable_shared_from_this<IArchiveEntry>
{
protected:
    std::weak_ptr<IArchiveEntry> mParent;
    std::list<IArchiveEntryPtr> mChildren;
    std::wstring mEntryName;  // Just the name, not full path

public:
    virtual ~IArchiveEntry() = default;

    std::shared_ptr<IArchiveEntry> getParent() const { return mParent.lock(); }

    // C# API: FileName property (returns narrow string)
    std::string getFileName() const {
        return std::string(mEntryName.begin(), mEntryName.end());
    }

    // Legacy API: getEntryName (returns wide string)
    std::wstring getEntryName() const { return mEntryName; }

    const std::list<IArchiveEntryPtr>& getChildren() const { return mChildren; }

    virtual bool isDirectory() const = 0;
    virtual bool hasChildren() const { return false; }

    // Get full path with backslashes (legacy behavior, wstring)
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

    // Get full path with forward slashes (C# behavior, narrow string)
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

// ============================================================================
// IArchiveFileEntry - Interface for file entries (matches C# interface)
// ============================================================================
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

// ============================================================================
// DirectoryEntry - Directory in the archive
// ============================================================================
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

    // Get all files recursively
    void getFiles(std::vector<IArchiveFileEntryPtr>& files);

    // Get all entries
    void getEntries(std::list<IArchiveEntryPtr>& entries, bool recursive = false);

    // Find entry by path (wstring version - legacy)
    IArchiveEntryPtr findEntry(const std::wstring& path);

    // Find entry by path (string version - C# API)
    IArchiveEntryPtr findEntry(const std::string& path);

    // Find file entry by path (wstring version)
    IArchiveFileEntryPtr findFileEntry(const std::wstring& path);

    // Find file entry by path (string version)
    IArchiveFileEntryPtr findFileEntry(const std::string& path);

    // Get files matching a pattern (supports * and ? wildcards)
    std::vector<IArchiveFileEntryPtr> getFiles(const std::string& pattern);
    std::vector<IArchiveFileEntryPtr> getFiles(const std::wstring& pattern);
};

// ============================================================================
// FileEntry - File in the archive
// ============================================================================
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

// ============================================================================
// IndexFile - Represents the index portion of the archive (matches C# IndexFile)
// ============================================================================
class IndexFile
{
    ArchivePtr mArchive;
    DirectoryEntryPtr mRoot;

public:
    IndexFile(ArchivePtr archive, DirectoryEntryPtr root);

    // C# API: FindEntry(string path) - returns IArchiveEntry (file or directory)
    IArchiveEntryPtr findEntry(const std::string& path);
    IArchiveEntryPtr findEntry(const std::wstring& path);

    // C# API: GetFiles(string pattern) - returns matching file entries
    std::vector<IArchiveFileEntryPtr> getFiles(const std::string& pattern);
    std::vector<IArchiveFileEntryPtr> getFiles(const std::wstring& pattern);

    // Get root directory
    DirectoryEntryPtr getRoot() const { return mRoot; }
};

// ============================================================================
// ArchiveFile - Represents a .archive data file (for CoreData support)
// ============================================================================
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

// ============================================================================
// Archive - Main archive class (matches C# Archive class)
// ============================================================================
class Archive : public std::enable_shared_from_this<Archive>
{
    std::filesystem::path mIndexPath;
    std::ifstream mIndexFile;
    std::ifstream mPackFile;

    // Index file data
    uint32 mDirectoryCount = 0;
    uint64 mDirectoryTableStart = 0;
    AIDX mIndexHeader;
    std::vector<PackDirectoryHeader> mDirectoryHeaders;
    DirectoryEntryPtr mFileRoot;
    uint32 mFileCount = 0;

    // Archive file data
    std::vector<PackDirectoryHeader> mPkDirectoryHeaders;
    std::vector<AARCEntry> mAarcTable;
    uint32 mPkDirCount = 0;
    uint64 mPkDirStart = 0;

    // Optional CoreData archive
    ArchiveFilePtr mCoreData;

    // Index file object (C# API)
    IndexFilePtr mIndexFileObj;

    void loadIndexTree();

public:
    Archive(const std::filesystem::path& indexPath, ArchiveFilePtr coreData = nullptr);

    // C# API: static Archive FromFile(string path, ArchiveFile coreData = null)
    static ArchivePtr fromFile(const std::filesystem::path& indexPath, ArchiveFilePtr coreData = nullptr);

    // Legacy loading methods (for backward compatibility - prefer fromFile() for new code)
    void loadIndexInfo();
    void loadArchiveInfo();
    void asyncLoad();  // Parses children after loading

    // C# API: IndexFile property
    IndexFilePtr getIndexFile() const { return mIndexFileObj; }

    // C# API: OpenFileStream(IArchiveFileEntry entry) - returns extracted data
    // In C++ we fill a vector instead of returning a stream
    bool openFileStream(IArchiveFileEntryPtr entry, std::vector<uint8>& content);
    bool openFileStream(FileEntryPtr entry, std::vector<uint8>& content);

    // Legacy API aliases for backward compatibility
    bool getFileData(IArchiveFileEntryPtr entry, std::vector<uint8>& content) { return openFileStream(entry, content); }
    bool getFileData(FileEntryPtr entry, std::vector<uint8>& content) { return openFileStream(entry, content); }

    // C# API: GetFileInfoByPath(string path) - returns IArchiveFileEntry
    IArchiveFileEntryPtr getFileInfoByPath(const std::string& path);
    IArchiveFileEntryPtr getFileInfoByPath(const std::wstring& path);

    // Legacy API: getByPath - returns any entry (file or directory)
    IArchiveEntryPtr getByPath(const std::wstring& path);
    IArchiveEntryPtr getByPath(const std::string& path);

    // Legacy API for compatibility
    DirectoryEntryPtr getRoot() const { return mFileRoot; }
    std::filesystem::path getPath() const { return mIndexPath; }
    uint32 getFileCount() const { return mFileCount; }

    // Internal access
    const std::vector<PackDirectoryHeader>& getBlockTable() const { return mDirectoryHeaders; }
    const std::vector<AARCEntry>& getAarcTable() const { return mAarcTable; }
    const std::vector<PackDirectoryHeader>& getPkDirectoryHeaders() const { return mPkDirectoryHeaders; }

    // Index file I/O helpers
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

    // Pack file I/O helpers
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

    // Magic constants
    static const uint32 FileMagic = 0x5041434B;         // 'PACK'
    static const uint32 IndexMagic = 0x41494458;        // 'AIDX'
    static const uint32 FileMagicArchive = 0x5041434B;  // 'PACK'
    static const uint32 AarcMagic = 0x41415243;         // 'AARC'
};

// ============================================================================
// Utility functions
// ============================================================================

// Convert pattern with wildcards to regex
std::string patternToRegex(const std::string& pattern);

// Normalize path separators to forward slashes
std::string normalizePath(const std::string& path);

// Check if path matches pattern
bool pathMatchesPattern(const std::string& path, const std::string& pattern);