#include "stdafx.h"
#include "BinLocale.h"
#include "IOManager.h"

void BinLocale::load() {
	sIOMgr->getArchive()->getFileData(mFile, mData);
	mStream = std::make_unique<BinStream>(mData);

	mHeader = mStream->read<Header>();

	mStream->seek(sizeof(Header) + mHeader.ofsEntries);
	std::vector<Entry> entries((size_t) mHeader.numEntries);
	mStream->read(entries.data(), entries.size() * sizeof(Entry));

	mPrototypes = entries;

	mEntries.resize(entries.size());
}

std::wstring BinLocale::loadRange(uint32 min, uint32 max) {
	min = std::min(mEntries.size(), min);
	max = std::min(mEntries.size(), max);

	if (min >= max) {
		return L"[ ]";
	}

	std::wstringstream strm;
	strm << L"[ ";

	for (uint32 i = min; i < max; ++i) {
		if (i != min) {
			strm << L", ";
		}

		if (mEntries[i].loaded == false) {
			std::wstring text = (wchar_t*) mStream->getPointer(sizeof(Header) + mHeader.ofsNames + mPrototypes[i].ofsEntry * sizeof(wchar_t));
			mEntries[i].text = text;
			mEntries[i].loaded = true;
			mEntries[i].id = mPrototypes[i].id;
		}

		strm << L"{ \"id\": " << mEntries[i].id << L", \"text\": \"" << escapeJsonString(mEntries[i].text) << L"\" }";
	}

	strm << L" ]";

	return strm.str();
}

void BinLocale::exportAsCsv(const std::wstring& file) {
	HANDLE hFile = CreateFile(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	std::string header = "id;LocalizedText\r\n";
	DWORD numWritten = 0;
	WriteFile(hFile, header.c_str(), header.length(), &numWritten, nullptr);
	
	for (uint32 i = 0; i < mEntries.size(); ++i) {
		auto& le = mEntries[i];
		if (mEntries[i].loaded == false) {
			std::wstring text = (wchar_t*) mStream->getPointer(sizeof(Header) + mHeader.ofsNames + mPrototypes[i].ofsEntry * sizeof(wchar_t));
			mEntries[i].text = text;
			mEntries[i].loaded = true;
			mEntries[i].id = mPrototypes[i].id;
		}

		std::wstringstream strm;
		strm << le.id << L";" << escapeCsvString(le.text) << L"\r\n";
		std::string str = String::toAnsi(strm.str());

		WriteFile(hFile, str.c_str(), str.length(), &numWritten, nullptr);
	}

	CloseHandle(hFile);
}