#pragma once

class String
{
public:
	static std::string toAnsi(const std::wstring& str);
	static std::wstring toUnicode(const std::string& str);

	static void split(const std::wstring& str, std::list<std::wstring>& elems, wchar_t delimeter);

	static bool endsWith(const std::wstring& str, const std::wstring& endStr);
	static bool isEqual(const std::wstring& s1, const std::wstring& s2, bool caseSensitive = false);

	static std::wstring toLower(const std::wstring& s);

	static inline std::wstring& ltrim(std::wstring& s);
	static inline std::wstring& rtrim(std::wstring& s);
	static std::wstring& trim(std::wstring& s);

	static uint32 hash(const std::wstring& s);
};