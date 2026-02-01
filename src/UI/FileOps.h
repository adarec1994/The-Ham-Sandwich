#pragma once

#include "../Archive.h"
#include "../export/M3Export.h"
#include "../BNK/BNK_hirc.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <unordered_set>

struct AppState;

namespace UI_ContentBrowser {

    struct FileInfo
    {
        std::string name;
        std::string extension;
        IFileSystemEntryPtr entry;
        ArchivePtr archive;
        bool isDirectory;
        ImTextureID textureID = 0;
        int texWidth = 0;
        int texHeight = 0;
        bool attemptedLoad = false;
        bool isLoadAllEntry = false;
        bool isTextureMapEntry = false;
    };

    struct BnkWemEntry {
        uint32_t id;
        uint32_t offset;
        uint32_t size;
        std::string displayName;
    };

    struct ThumbnailRequest {
        ArchivePtr archive;
        std::shared_ptr<FileEntry> entry;
        std::string extension;
        int fileIndex;
        uint64_t generation;
        bool isComposite = false;
        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> compositeAreaFiles;
        bool isTextureComposite = false;
        std::vector<std::pair<ArchivePtr, std::shared_ptr<FileEntry>>> compositeTexFiles;
    };

    struct ThumbnailResult {
        int width;
        int height;
        std::vector<uint8_t> data;
        int fileIndex;
        uint64_t generation;
        bool success;
    };

    extern bool sIsOpen;
    extern bool sIsDocked;
    extern float sCurrentHeight;
    extern float sTargetHeight;
    extern float sAnimSpeed;
    extern float sTreeWidth;
    extern ImTextureID sFolderIcon;
    extern ImTextureID sContentBrowserIcon;
    extern ImTextureID sTblIcon;
    extern ImTextureID sAreaIcon;
    extern ImTextureID sAudioIcon;

    extern IFileSystemEntryPtr sSelectedFolder;
    extern ArchivePtr sSelectedArchive;
    extern std::string sSelectedPath;

    extern char sSearchFilter[256];

    extern std::vector<FileInfo> sCachedFiles;
    extern bool sNeedsRefresh;
    extern int sSelectedFileIndex;

    extern bool sRequestTreeSync;
    extern std::unordered_set<const void*> sNodesToExpand;

    extern std::vector<IFileSystemEntryPtr> sBreadcrumbPath;

    extern ArchivePtr sExportArchive;
    extern std::shared_ptr<FileEntry> sExportFileEntry;
    extern std::string sExportDefaultName;
    extern std::string sLastExportPath;
    extern std::atomic<bool> sExportInProgress;
    extern std::atomic<int> sExportProgress;
    extern std::atomic<int> sExportTotal;
    extern std::string sExportStatus;
    extern std::mutex sExportMutex;
    extern M3Export::ExportResult sExportResult;
    extern bool sShowExportResult;
    extern float sNotificationTimer;
    extern std::string sNotificationMessage;
    extern bool sNotificationSuccess;

    extern std::vector<uint8_t> sAudioExportData;
    extern std::string sAudioExportName;

    extern std::vector<uint8_t> sBnkExportData;
    extern std::string sBnkExportName;

    extern bool sBnkViewActive;
    extern std::vector<uint8_t> sBnkViewData;
    extern std::string sBnkViewName;
    extern ArchivePtr sBnkViewArchive;
    extern std::vector<BnkWemEntry> sBnkWemEntries;

    extern std::vector<uint8_t> sTblExportData;
    extern std::string sTblExportName;

    extern Bnk::WemNameResolver sWemResolver;

    extern std::deque<ThumbnailRequest> sLoadQueue;
    extern std::deque<ThumbnailResult> sResultQueue;
    extern std::mutex sQueueMutex;
    extern std::atomic<bool> sWorkerRunning;
    extern std::thread sWorkerThread;
    extern uint64_t sCurrentGeneration;
    extern std::condition_variable sQueueCV;

    void InitializeAudioDatabase(AppState& state);
    void LoadWemNameLookup(const std::vector<ArchivePtr>& archives);
    std::string GetWemDisplayName(uint32_t id);

    bool BuildPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target, std::vector<IFileSystemEntryPtr>& outPath);
    void ThumbnailWorker();
    void EnsureWorkerStarted();
    void ProcessThumbnailResults();
    std::string GetExtension(const std::string& filename);
    bool FindPathToNode(const IFileSystemEntryPtr& current, const IFileSystemEntryPtr& target);
    void CollectRecursive(const ArchivePtr& archive, const IFileSystemEntryPtr& folder, const std::string& filterLower, std::vector<FileInfo>& outList);
    void RefreshFileList(AppState& state);
    void LoadSingleArea(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry);
    void LoadAllAreasInFolder(AppState& state);
    void UpdateLoadAllAreas(AppState& state);
    void DrawLoadAllProgress();
    bool IsLoadAllInProgress();
    void LoadSingleM3(AppState& state, const ArchivePtr& arc, const std::shared_ptr<FileEntry>& fileEntry);
    void HandleFileOpen(AppState& state, const FileInfo& file);
    IFileSystemEntryPtr FindFolderByPath(const IFileSystemEntryPtr& root, const std::vector<std::string>& pathParts, size_t startIndex = 0);
    std::vector<std::string> SplitPath(const std::string& path);
    std::string ExtractFolderPath(const std::string& filePath);

}