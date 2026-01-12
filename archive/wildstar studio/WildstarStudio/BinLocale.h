#pragma once

#include "Archive.h"

enum class Language : uint32
{
	English = 1,
	German,
	French,
	Korean
};

class BinLocale
{
	FileEntryPtr mFile;

	struct Header
	{
		uint32 magic;
		uint32 version;
		Language language;
		uint32 unk1;
		uint64 lenTagName;
		uint64 ofsTagName;
		uint64 lenShortName;
		uint64 ofsShortName;
		uint64 lenLongName;
		uint64 ofsLongName;
		uint64 numEntries;
		uint64 ofsEntries;
		uint64 lenNames;
		uint64 ofsNames;
	};

	struct Entry
	{
		uint32 id;
		uint32 ofsEntry;
	};
	
	struct LocaleEntry
	{
		LocaleEntry() {
			id = 0;
			text = L"";
			loaded = false;
		}

		uint32 id;
		std::wstring text;
		bool loaded;
	};

	Header mHeader;

	std::vector<LocaleEntry> mEntries;
	std::vector<Entry> mPrototypes;
	std::vector<uint8> mData;
	std::unique_ptr<BinStream> mStream;

	std::wstring escapeJsonString(const std::wstring& input) {
		std::wostringstream ss;
		for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
			if (std::iswalnum(*iter) == false && std::iswprint(*iter) == false) {
				continue;
			}

			switch (*iter) {
			case L'\\': ss << L"\\\\"; break;
			case L'"': ss << L"\\\""; break;
			case L'/': ss << L"\\/"; break;
			case L'\b': ss << L"\\b"; break;
			case L'\f': ss << L"\\f"; break;
			case L'\n': ss << L"\\n"; break;
			case L'\r': ss << L"\\r"; break;
			case L'\t': ss << L"\\t"; break;
			default: ss << *iter; break;
			}
		}
		return ss.str();
	}

	std::wstring escapeCsvString(const std::wstring& input) {
		std::wostringstream ss;
		for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
			if (std::iswalnum(*iter) == false && std::iswprint(*iter) == false) {
				continue;
			}

			switch (*iter) {
			case L'\\': ss << L"\\\\"; break;
			case L'/': ss << L"\\/"; break;
			case L'\b': ss << L"\\b"; break;
			case L'\f': ss << L"\\f"; break;
			case L'\n': ss << L"\\n"; break;
			case L'\r': ss << L"\\r"; break;
			case L'\t': ss << L"\\t"; break;
			case L';': ss << L"\\;"; break;
			default: ss << *iter; break;
			}
		}

		return ss.str();
	}

public:
	BinLocale(FileEntryPtr file) { mFile = file; }

	void load();

	std::wstring loadRange(uint32 start, uint32 end);

	void exportAsCsv(const std::wstring& file);

	const Header& getHeader() const { return mHeader; }
};

SHARED_TYPE(BinLocale);