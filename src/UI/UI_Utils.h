#pragma once
#include <string>

std::string wstring_to_utf8(const std::wstring& str);
std::string ToLowerCopy(std::string s);
bool EndsWithNoCase(const std::string& s, const std::string& suffix);
std::string GetExtLower(const std::string& name);
bool ContainsLowerFast(const std::string& hayLower, const std::string& needleLower);
