#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace Audio {

    class AudioExport {
    public:
        static bool ExportWEMToWAV(const uint8_t* wemData, size_t wemSize, const std::string& filepath);
        static bool ExportWEMToWAV(const uint8_t* wemData, size_t wemSize, uint32_t wemId, const std::string& outputDir, std::string& outFilepath);
        static bool ExportWEMRaw(const uint8_t* wemData, size_t wemSize, uint32_t wemId, const std::string& outputDir, std::string& outFilepath);
        static std::string GetExportFilename(uint32_t wemId, const std::string& extension);
        static const std::string& GetLastError() { return s_lastError; }

    private:
        static std::string s_lastError;
        static std::string SanitizeFilename(const std::string& name);
    };

}