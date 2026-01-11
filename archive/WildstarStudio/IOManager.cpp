#include "stdafx.h"
#include "IOManager.h"
#include "UIManager.h"
#include "BinLocale.h"

std::shared_ptr<IOManager> IOManager::gInstance;

IOManager::IOManager() {
	mIsAsyncLoading = false;
	mAsyncLoadCount = 0;
	queryExtractionPath();
}

void IOManager::loadFromPath(const std::wstring& path) {
	mAsyncLoadCount = 0;
	mArchive = std::make_shared<Archive>(path);
	mArchive->loadIndexInfo();
	mArchive->loadArchiveInfo();
}

void IOManager::doAsyncLoad(std::function<void ()> callback) {
	mIsAsyncLoading = true;

	std::async(std::launch::async,
		[this, callback]() {
			mArchive->asyncLoad();
			mIsAsyncLoading = false;
			callback();
		}
	);
}

void IOManager::asyncFileLoaded() {
	++mAsyncLoadCount;
}

void IOManager::queryExtractionPath() {
	HKEY hBaseKey = nullptr;
	auto res = RegOpenKey(HKEY_CURRENT_USER, L"Software\\Cromon\\WildstarStudio", &hBaseKey);
	if (res != ERROR_SUCCESS) {
		return loadDefaultPath();
	}

	std::vector<wchar_t> folder(MAX_PATH + 1);
	ULONG maxLen = MAX_PATH * sizeof(wchar_t);
	DWORD dwType = 0;
	res = RegQueryValueEx(hBaseKey, L"ExtractionPath", nullptr, &dwType, (LPBYTE) folder.data(), &maxLen);
	if (res != ERROR_SUCCESS || dwType != REG_SZ) {
		return loadDefaultPath();
	}

	if ((GetFileAttributes(folder.data()) & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		return loadDefaultPath();
	}

	mExtractionPath = folder.data();
	if (String::endsWith(mExtractionPath, L"\\") == false) {
		mExtractionPath += L"\\";
	}
}

void IOManager::loadDefaultPath() {
	std::wstringstream strm;
	wchar_t curDir[MAX_PATH + 1] = { L'\0' };
	GetCurrentDirectory(MAX_PATH, curDir);
	strm << curDir << L"\\out\\";

	mExtractionPath = strm.str();

	saveExtractionPath();
}

void IOManager::saveExtractionPath() {
	if (String::endsWith(mExtractionPath, L"\\") == false) {
		mExtractionPath += L"\\";
	}

	HKEY baseKey = nullptr;
	auto res = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Cromon\\WildstarStudio", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &baseKey, nullptr);
	if (res != ERROR_SUCCESS) {
		return;
	}

	RegSetValueEx(baseKey, L"ExtractionPath", 0, REG_SZ, (LPCBYTE) mExtractionPath.c_str(), mExtractionPath.length() * sizeof(wchar_t));
}

void IOManager::extractEntriesByFilter(const std::wstring& path, const FilterParameters& params) {
	auto regex = params.regex;
	if (regex.length() == 0) {
		regex = L".*";
	}

	std::wregex rgx(regex);

	std::async(std::launch::async, [path, params, this, rgx]() {
		auto entry = path.empty() ? mArchive->getRoot() : mArchive->getByPath(path);
		if (entry == nullptr) {
			sUIMgr->asyncExtractComplete();
			return;
		}

		if (entry->isDirectory() == false) {
			std::wsmatch match;
			if (std::regex_search(path, match, rgx)) {
				filteredExtractFile(std::dynamic_pointer_cast<FileEntry>(entry), params);
			}
		} else {
			filteredExtractDirectory(std::dynamic_pointer_cast<DirectoryEntry>(entry), params, rgx);
		}

		sUIMgr->asyncExtractComplete();
	});
}

void IOManager::filteredExtractDirectory(DirectoryEntryPtr dir, const FilterParameters& params, const std::wregex& regex) {
	for (auto& child : dir->getChildren()) {
		if (child->isDirectory()) {
			filteredExtractDirectory(std::dynamic_pointer_cast<DirectoryEntry>(child), params, regex);
		} else {
			// extractFile(std::dynamic_pointer_cast<FileEntry>(child));
			auto file = std::dynamic_pointer_cast<FileEntry>(child);
			auto path = file->getFullPath();
			std::wsmatch match;
			if (std::regex_search(path, match, regex)) {
				filteredExtractFile(file, params);
			}
		}
	}
}

void IOManager::filteredExtractFile(FileEntryPtr file, const FilterParameters& params) {
	std::wstring path = file->getFullPath();
	std::wstringstream strm;
	strm << mExtractionPath << path;
	std::tr2::sys::path wp(strm.str());
	std::wstring dir = wp.parent_path();

	auto ext = wp.extension().wstring();
	std::transform(ext.begin(), ext.end(), ext.begin(), towupper);

	bool hasExtension = false;
	for (auto& dstExt : params.extensions) {
		if (dstExt == ext) {
			hasExtension = true;
			break;
		}
	}

	if (!hasExtension) {
		return;
	}

	SHCreateDirectoryEx(nullptr, dir.c_str(), nullptr);

	if (String::toLower(wp.extension()) == L".tex" && params.texAsBmp) {
		Texture tex(file);
		auto bmp = tex.getBitmap();
		CLSID clsId;
		GetEncoderClsid(L"image/bmp", &clsId);

		strm << L".bmp";
		bmp->Save(strm.str().c_str(), &clsId);
	} else if (String::toLower(wp.extension()) == L".tbl") {
		std::wstring oldPart = strm.str();
		if (params.tblAsCsv) {
			strm << L".csv";
			DataTable dtbl(file);
			dtbl.initialLoad();
			dtbl.exportAsCsv(strm.str());
		} else {
			std::ofstream ostr(strm.str(), std::ios::binary);
			std::vector<uint8> content;
			mArchive->getFileData(file, content);
			ostr.write((char*) content.data(), content.size());
			ostr.close();
		}
	} else if (String::toLower(wp.extension()) == L".area" && params.areaAsObj) {
		AreaFile area(path, file);
		if (area.loadForExport()) {
			area.exportToObj(true);
		}
	} else if (String::toLower(wp.extension()) == L".bin" && params.tblAsCsv) {
		strm << L".csv";
		BinLocale l(file);
		l.load();
		l.exportAsCsv(strm.str());
	} else if (String::toLower(wp.extension()) == L".m3" && params.m3AsObj) {
		M3Model model(path, file);
		model.loadForExport();
		model.exportAsObj(true);
	} else {
		std::ofstream ostr(strm.str(), std::ios::binary);
		std::vector<uint8> content;
		mArchive->getFileData(file, content);
		ostr.write((char*) content.data(), content.size());
		ostr.close();
	}
}

void IOManager::extractEntry(const std::wstring& path) {
	auto entry = mArchive->getByPath(path);
	if (entry == nullptr) {
		sUIMgr->asyncExtractComplete();
	}

	std::async(std::launch::async, [entry, this]() {
		if (entry->isDirectory() == false) {
			extractFile(std::dynamic_pointer_cast<FileEntry>(entry));
		} else {
			DirectoryEntryPtr dirent = std::dynamic_pointer_cast<DirectoryEntry>(entry);
			extractDirectory(dirent);
		}

		sUIMgr->asyncExtractComplete();
	});
}

void IOManager::extractDirectory(DirectoryEntryPtr dirent) {
	for (auto& child : dirent->getChildren()) {
		if (child->isDirectory()) {
			extractDirectory(std::dynamic_pointer_cast<DirectoryEntry>(child));
		} else {
			extractFile(std::dynamic_pointer_cast<FileEntry>(child));
		}
	}
}

void IOManager::extractFile(FileEntryPtr file) {
	std::wstring path = file->getFullPath();
	std::wstringstream strm;
	strm << mExtractionPath << path;
	std::tr2::sys::path wp(strm.str());
	std::wstring dir = wp.parent_path();

	SHCreateDirectoryEx(nullptr, dir.c_str(), nullptr);

	if (String::toLower(wp.extension()) == L".tex" && sUIMgr->extractTextureAsBmp()) {
		Texture tex(file);
		auto bmp = tex.getBitmap();
		CLSID clsId;
		GetEncoderClsid(L"image/bmp", &clsId);

		strm << L".bmp";
		bmp->Save(strm.str().c_str(), &clsId);
	} else if (String::toLower(wp.extension()) == L".tbl") {
		std::wstring oldPart = strm.str();
		if (sUIMgr->extractTblAsCsv()) {
			strm << L".csv";
			DataTable dtbl(file);
			dtbl.initialLoad();
			dtbl.exportAsCsv(strm.str());
		}

		if (sUIMgr->extractTblAsSql()) {
			strm = std::wstringstream();
			strm << oldPart << L".sql";
			DataTable dtbl(file);
			dtbl.initialLoad();
			dtbl.exportAsSql(strm.str());
		}

		if (sUIMgr->extractTblAsCsv() == false && sUIMgr->extractTblAsSql() == false) {
			std::ofstream ostr(strm.str(), std::ios::binary);
			std::vector<uint8> content;
			mArchive->getFileData(file, content);
			ostr.write((char*) content.data(), content.size());
			ostr.close();
		}
	} else if (String::toLower(wp.extension()) == L".bin" && sUIMgr->extractTblAsCsv()) {
		strm << L".csv";
		BinLocale l(file);
		l.load();
		l.exportAsCsv(strm.str());
	} else {
		std::ofstream ostr(strm.str(), std::ios::binary);
		std::vector<uint8> content;
		mArchive->getFileData(file, content);
		ostr.write((char*) content.data(), content.size());
		ostr.close();
	}
}