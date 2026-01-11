#include "StdAfx.h"
#include "String.h"

std::string String::toAnsi(const std::wstring& str) {
	int numBytes = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), nullptr, 0, nullptr, nullptr);
	std::vector<char> ansi(numBytes + 1);
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), ansi.data(), numBytes, nullptr, nullptr);
	ansi.push_back('\0');
	return ansi.data();
}

std::wstring String::toUnicode(const std::string& str) {
	int numBytes = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), nullptr, 0);
	std::vector<wchar_t> ansi(numBytes + 1);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), ansi.data(), numBytes);
	ansi.push_back(L'\0');
	return ansi.data();
}

std::wstring String::toLower(const std::wstring& s) {
	std::wstring lower;
	std::transform(s.begin(), s.end(), std::back_inserter(lower), towlower);
	return lower;
}

void String::split(const std::wstring& str, std::list<std::wstring>& elems, wchar_t delim) {
	std::wstringstream ss(str);
	std::wstring item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

bool String::endsWith(const std::wstring& str, const std::wstring& endStr) {
	return str.rfind(endStr) == (str.length() - endStr.length());
}

bool String::isEqual(const std::wstring& s1, const std::wstring& s2, bool caseSensitive) {
	if (s1.length() != s2.length())
		return false;

	for (uint32 i = 0; i < s1.length(); ++i) {
		wchar_t c1 = s1[i];
		wchar_t c2 = s2[i];

		if (caseSensitive == false) {
			c1 = towlower(c1);
			c2 = towlower(c2);
		}

		if (c1 != c2)
			return false;
	}

	return true;
}

std::wstring& String::trim(std::wstring& s) {
	return ltrim(rtrim(s));
}

std::wstring& String::ltrim(std::wstring& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::wstring& String::rtrim(std::wstring& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

uint32 String::hash(const std::wstring& s) {
	static std::hash<std::wstring> stringHash;

	return stringHash(s);
}