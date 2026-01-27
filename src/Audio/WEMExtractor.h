#pragma once

#include "BNKReader.h"
#include <functional>

namespace Audio
{
    using ExtractProgressCallback = std::function<void(size_t current, size_t total, const std::string& filename)>;

    class WEMExtractor
    {
    public:
        WEMExtractor() = default;
        ~WEMExtractor() = default;

        int ExtractAll(const SoundBank& bank, const std::string& outputDir,
                       ExtractProgressCallback callback = nullptr);

        bool ExtractById(const SoundBank& bank, uint32_t wemId, const std::string& outputPath);
        bool ExtractByIndex(const SoundBank& bank, size_t index, const std::string& outputPath);

        static std::string GetWEMFilename(const SoundBank& bank, uint32_t wemId);

        const std::string& GetLastError() const { return m_lastError; }

    private:
        bool SaveWEMToFile(const std::vector<uint8_t>& data, const std::string& path);
        bool CreateDirectory(const std::string& path);

        std::string m_lastError;
    };

    struct WEMInfo
    {
        uint32_t riffSize = 0;
        uint32_t fmtSize = 0;
        uint16_t formatTag = 0;
        uint16_t channels = 0;
        uint32_t sampleRate = 0;
        uint32_t avgBytesPerSec = 0;
        uint16_t blockAlign = 0;
        uint16_t bitsPerSample = 0;
        uint32_t dataSize = 0;
        bool isValid = false;

        bool IsVorbis() const { return formatTag == 0xFFFF || formatTag == 0xFFFE; }
        bool IsPCM() const { return formatTag == 0x0001; }
        bool IsADPCM() const { return formatTag == 0x0002 || formatTag == 0x0011 || formatTag == 0x0069; }
        bool IsFloat() const { return formatTag == 0x0003; }
    };

    bool ParseWEMHeader(const uint8_t* data, size_t size, WEMInfo& outInfo);
    bool ParseWEMHeader(const std::vector<uint8_t>& data, WEMInfo& outInfo);

}