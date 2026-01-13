#pragma once

#include "Archive.h"

struct FilterParameters
{
	std::list<std::wstring> extensions;
	std::wstring regex;
	bool areaAsObj;
	bool texAsBmp;
	bool m3AsObj;
	bool tblAsCsv;
};

class IOManager
{
private:
	static std::shared_ptr<IOManager> gInstance;

	ArchivePtr mArchive;
	bool mIsAsyncLoading;
	std::mutex mLoadLock;
	std::atomic<uint32> mAsyncLoadCount;
	std::wstring mExtractionPath;

	void queryExtractionPath();
	void loadDefaultPath();
	void saveExtractionPath();

	void extractFile(FileEntryPtr file);
	void extractDirectory(DirectoryEntryPtr dir);

	void filteredExtractDirectory(DirectoryEntryPtr dir, const FilterParameters& params, const std::wregex& regex);
	void filteredExtractFile(FileEntryPtr file, const FilterParameters& params);
public:
	IOManager();

	void loadFromPath(const std::wstring& path);
	void doAsyncLoad(std::function<void ()> callback);

	void extractEntry(const std::wstring& entry);
	void extractEntriesByFilter(const std::wstring& entry, const FilterParameters& params);

	void setExtractionPath(const std::wstring& path) { mExtractionPath = path; saveExtractionPath(); }
	std::wstring getExtractionPath() const { return mExtractionPath; }

	void asyncFileLoaded();
	uint32 getAsyncLoadCount() const { return mAsyncLoadCount; }

	bool isAsyncUpdating() const { return mIsAsyncLoading; }

	ArchivePtr getArchive() const { return mArchive; }

	static std::shared_ptr<IOManager> getInstance() {
		if (gInstance == nullptr) {
			gInstance = std::make_shared<IOManager>();
		}

		return gInstance;
	}
};

#define sIOMgr (IOManager::getInstance())