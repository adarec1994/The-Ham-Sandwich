#pragma once

#include <exception>
#include <string>

namespace DebugLog
{
    void Init();
    void Write(const std::string& category, const std::string& message);
    void WriteException(const std::string& category, const std::exception& e);
    void WriteUnknownException(const std::string& category);
}
