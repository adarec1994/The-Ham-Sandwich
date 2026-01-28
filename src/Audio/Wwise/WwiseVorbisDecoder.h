#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace Audio {

    bool WwiseInitEmbedded();
    bool WwiseInit(const std::string& codebooksPath, const std::string& logPath = "");
    void WwiseShutdown();

    class WwiseVorbisDecoder {
    public:
        bool ParseWEM(const uint8_t* data, size_t size);
        bool ConvertToOgg(std::vector<uint8_t>& outOgg);

        uint32_t GetChannels() const { return m_channels; }
        uint32_t GetSampleRate() const { return m_sampleRate; }
        uint32_t GetSampleCount() const { return m_sampleCount; }
        const std::string& GetLastError() const { return m_lastError; }

    private:
        const uint8_t* m_data = nullptr;
        size_t m_size = 0;
        bool m_littleEndian = true;
        std::string m_lastError;

        long m_fmtOffset = -1, m_fmtSize = -1;
        long m_vorbOffset = -1, m_vorbSize = -1;
        long m_dataOffset = -1, m_dataSize = -1;

        uint16_t m_channels = 0;
        uint32_t m_sampleRate = 0;
        uint32_t m_avgBytesPerSec = 0;

        uint32_t m_sampleCount = 0;
        uint32_t m_setupPacketOffset = 0;
        uint32_t m_firstAudioPacketOffset = 0;
        uint8_t m_blocksize0Pow = 0;
        uint8_t m_blocksize1Pow = 0;

        bool m_noGranule = false;
        bool m_modPackets = false;

        std::vector<bool> m_modeBlockflag;
        int m_modeBits = 0;
    };

}