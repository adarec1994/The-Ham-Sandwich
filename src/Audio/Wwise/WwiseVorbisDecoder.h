#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace Audio {

// Matches vgmstream's wwise_setup_t
enum class WwiseSetupType {
    HeaderTriad,        // WWV_HEADER_TRIAD
    FullSetup,          // WWV_FULL_SETUP
    InlineCodebooks,    // WWV_INLINE_CODEBOOKS
    ExternalCodebooks,  // WWV_EXTERNAL_CODEBOOKS
    AoTuV603Codebooks   // WWV_AOTUV603_CODEBOOKS
};

// Matches vgmstream's wwise_header_t
enum class WwiseHeaderType {
    Type8,  // 4+4 bytes (size + granule)
    Type6,  // 2+4 bytes
    Type2   // 2 bytes (size only)
};

// Matches vgmstream's wwise_packet_t
enum class WwisePacketType {
    Standard,
    Modified
};

struct WwiseConfig {
    int channels = 0;
    int sampleRate = 0;
    int blocksize0Exp = 8;
    int blocksize1Exp = 11;
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;
    uint32_t setupOffset = 0;
    uint32_t audioOffset = 0;
    uint32_t totalSamples = 0;
    bool bigEndian = false;
    WwiseSetupType setupType = WwiseSetupType::AoTuV603Codebooks;
    WwiseHeaderType headerType = WwiseHeaderType::Type2;
    WwisePacketType packetType = WwisePacketType::Modified;
};

class WwiseVorbisDecoder {
public:
    bool ParseWEM(const uint8_t* data, size_t size);
    bool ConvertToOgg(std::vector<uint8_t>& outOgg);

    int GetSampleRate() const { return m_config.sampleRate; }
    int GetChannels() const { return m_config.channels; }
    uint32_t GetTotalSamples() const { return m_config.totalSamples; }
    const std::string& GetLastError() const { return m_lastError; }
    const WwiseConfig& GetConfig() const { return m_config; }

private:
    bool ParseFmtChunk(const uint8_t* data, size_t size);

    std::vector<uint8_t> BuildIdentificationHeader();
    std::vector<uint8_t> BuildCommentHeader();
    std::vector<uint8_t> BuildSetupHeader();

    size_t GetPacketHeader(size_t offset, uint16_t& packetSize, int32_t& granule);
    std::vector<uint8_t> RebuildAudioPacket(size_t offset, size_t dataEnd);

    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    std::string m_lastError;
    WwiseConfig m_config;

    // Mode info for packet reconstruction
    std::vector<bool> m_modeBlockflag;
    int m_modeBits = 0;
    bool m_prevBlockflag = false;
};

} // namespace Audio