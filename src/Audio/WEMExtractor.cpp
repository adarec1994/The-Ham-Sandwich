#include "WEMExtractor.h"
#include <fstream>
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

namespace Audio
{
    static inline uint16_t ReadU16(const uint8_t* data)
    {
        return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    }

    static inline uint32_t ReadU32(const uint8_t* data)
    {
        return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    }

    int WEMExtractor::ExtractAll(const SoundBank& bank, const std::string& outputDir,
                                  ExtractProgressCallback callback)
    {
        if (!CreateDirectory(outputDir))
        {
            m_lastError = "Failed to create output directory: " + outputDir;
            return -1;
        }

        const auto& descriptors = bank.GetWEMDescriptors();
        size_t total = descriptors.size();
        int extracted = 0;

        for (size_t i = 0; i < total; ++i)
        {
            const auto& desc = descriptors[i];
            std::string filename = GetWEMFilename(bank, desc.id);
            std::string fullPath = outputDir + "/" + filename;

            if (callback)
                callback(i + 1, total, filename);

            if (ExtractByIndex(bank, i, fullPath))
                ++extracted;
        }

        return extracted;
    }

    bool WEMExtractor::ExtractById(const SoundBank& bank, uint32_t wemId, const std::string& outputPath)
    {
        std::vector<uint8_t> data;
        if (!bank.ExtractWEM(wemId, data))
        {
            m_lastError = "Failed to extract WEM ID: " + std::to_string(wemId);
            return false;
        }

        return SaveWEMToFile(data, outputPath);
    }

    bool WEMExtractor::ExtractByIndex(const SoundBank& bank, size_t index, const std::string& outputPath)
    {
        std::vector<uint8_t> data;
        if (!bank.ExtractWEMByIndex(index, data))
        {
            m_lastError = "Failed to extract WEM at index: " + std::to_string(index);
            return false;
        }

        return SaveWEMToFile(data, outputPath);
    }

    std::string WEMExtractor::GetWEMFilename(const SoundBank& bank, uint32_t wemId)
    {
        const std::string* name = bank.GetStringById(wemId);
        if (name && !name->empty())
        {
            return *name + ".wem";
        }

        std::ostringstream ss;
        ss << wemId << ".wem";
        return ss.str();
    }

    bool WEMExtractor::SaveWEMToFile(const std::vector<uint8_t>& data, const std::string& path)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            m_lastError = "Failed to create file: " + path;
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();

        return true;
    }

    bool WEMExtractor::CreateDirectory(const std::string& path)
    {
        int result = MKDIR(path.c_str());
        return (result == 0 || errno == EEXIST);
    }

    bool ParseWEMHeader(const uint8_t* data, size_t size, WEMInfo& outInfo)
    {
        outInfo = WEMInfo();

        if (size < 12)
            return false;

        if (std::memcmp(data, "RIFF", 4) != 0)
            return false;

        outInfo.riffSize = ReadU32(data + 4);

        if (std::memcmp(data + 8, "WAVE", 4) != 0)
            return false;

        size_t offset = 12;

        while (offset + 8 <= size)
        {
            char chunkId[5] = {0};
            std::memcpy(chunkId, data + offset, 4);
            uint32_t chunkSize = ReadU32(data + offset + 4);
            offset += 8;

            if (offset + chunkSize > size)
                break;

            if (std::memcmp(chunkId, "fmt ", 4) == 0)
            {
                outInfo.fmtSize = chunkSize;

                if (chunkSize >= 16)
                {
                    outInfo.formatTag = ReadU16(data + offset);
                    outInfo.channels = ReadU16(data + offset + 2);
                    outInfo.sampleRate = ReadU32(data + offset + 4);
                    outInfo.avgBytesPerSec = ReadU32(data + offset + 8);
                    outInfo.blockAlign = ReadU16(data + offset + 12);
                    outInfo.bitsPerSample = ReadU16(data + offset + 14);
                }
            }
            else if (std::memcmp(chunkId, "data", 4) == 0)
            {
                outInfo.dataSize = chunkSize;
            }

            offset += chunkSize;
            if (offset % 2 != 0)
                offset++;
        }

        outInfo.isValid = (outInfo.fmtSize > 0);
        return outInfo.isValid;
    }

    bool ParseWEMHeader(const std::vector<uint8_t>& data, WEMInfo& outInfo)
    {
        return ParseWEMHeader(data.data(), data.size(), outInfo);
    }

}