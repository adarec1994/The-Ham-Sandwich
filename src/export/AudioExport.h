#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace Audio {

    class AudioExport {
    public:
        static bool ExportWEMToWAV(const uint8_t* wemData, size_t wemSize, const std::string& filepath);
        static const std::string& GetLastError() { return s_lastError; }

    private:
        static std::string s_lastError;
    };

}