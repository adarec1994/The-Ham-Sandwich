#pragma once
// WwiseVorbisDecoder.h - Wwise Vorbis to PCM decoder
// Based on hcs64's ww2ogg and vgmstream's implementation
// 
// Usage:
// 1. Call InitializeCodebooks() once at startup with path to packed_codebooks_aoTuV_603.bin
// 2. Create WwiseVorbisDecoder instance
// 3. Call ParseWEM() with file data
// 4. Call DecodeToOgg() to get standard OGG or DecodeToPCM() to get PCM samples

#include <cstdint>
#include <vector>
#include <string>

namespace Audio
{

// Wwise format version/header types
enum class WwiseHeaderType
{
    Type8,  // 8-byte packet headers (oldest, ~2008-2009)
    Type6,  // 6-byte packet headers (2009-2011)
    Type2   // 2-byte packet headers (2012+, most common)
};

enum class WwiseSetupType
{
    HeaderTriad,        // Full Vorbis headers in file (v34)
    FullSetup,          // Full setup packet (v38)
    InlineCodebooks,    // Codebooks inline in simplified format (v44)
    ExternalCodebooks,  // External codebook reference - standard (v48-v52)
    AoTuV603Codebooks   // External codebook reference - aoTuV 603 (v62+)
};

enum class WwisePacketType
{
    Standard,   // Standard Vorbis packets (unchanged)
    Modified    // Modified packets (mode/window info stripped)
};

// Decoded WEM configuration
struct WwiseVorbisConfig
{
    // Audio properties
    int channels = 0;
    int sampleRate = 0;
    uint32_t sampleCount = 0;
    
    // Block sizes (log2)
    uint8_t blocksize0Exp = 8;   // Small block (typically 256)
    uint8_t blocksize1Exp = 11;  // Large block (typically 2048)
    
    // Wwise version info
    WwiseHeaderType headerType = WwiseHeaderType::Type2;
    WwiseSetupType setupType = WwiseSetupType::AoTuV603Codebooks;
    WwisePacketType packetType = WwisePacketType::Modified;
    
    // File layout
    uint32_t dataOffset = 0;     // Start of audio data chunk
    uint32_t dataSize = 0;       // Size of audio data
    uint32_t setupOffset = 0;    // Offset to setup packet within data
    uint32_t firstAudioOffset = 0; // Offset to first audio packet
    
    // Endianness
    bool bigEndian = false;
};

// Main decoder class
class WwiseVorbisDecoder
{
public:
    WwiseVorbisDecoder();
    ~WwiseVorbisDecoder();
    
    // Parse WEM file header and extract configuration
    // Returns true if successful
    bool ParseWEM(const uint8_t* data, size_t size);
    
    // Convert to standard OGG Vorbis format
    // Returns true if successful, OGG data in outOgg
    bool ConvertToOgg(std::vector<uint8_t>& outOgg);
    
    // Decode to PCM samples (requires stb_vorbis)
    // Returns number of samples per channel, or -1 on error
    int DecodeToPCM(std::vector<int16_t>& outSamples);
    
    // Get configuration after parsing
    const WwiseVorbisConfig& GetConfig() const { return m_config; }
    
    // Get last error message
    const std::string& GetLastError() const { return m_lastError; }
    
    // Convenience accessors
    int GetSampleRate() const { return m_config.sampleRate; }
    int GetChannels() const { return m_config.channels; }
    uint32_t GetSampleCount() const { return m_config.sampleCount; }

private:
    // Build Vorbis identification header
    std::vector<uint8_t> BuildIdentificationHeader();
    
    // Build Vorbis comment header  
    std::vector<uint8_t> BuildCommentHeader();
    
    // Build Vorbis setup header (the complex part)
    std::vector<uint8_t> BuildSetupHeader();
    
    // Read a Wwise packet header
    bool ReadPacketHeader(size_t offset, uint16_t& packetSize, int32_t& granulePos);
    
    // Convert a single Wwise audio packet to standard Vorbis
    bool ConvertAudioPacket(const uint8_t* input, size_t inputSize,
                           std::vector<uint8_t>& output, bool hasNext, uint8_t nextByte);
    
    // Rebuild codebook from external reference
    bool RebuildCodebook(uint32_t codebookId, std::vector<uint8_t>& output);
    
    // Copy codebook as-is (full setup mode)
    bool CopyCodebook(const uint8_t*& input, size_t& inputRemaining, std::vector<uint8_t>& output);
    
    // Rebuild codebook from inline data
    bool RebuildInlineCodebook(const uint8_t*& input, size_t& inputRemaining, std::vector<uint8_t>& output);
    
    WwiseVorbisConfig m_config;
    std::string m_lastError;
    std::vector<uint8_t> m_rawData;
    
    // Mode info for packet conversion (parsed from setup)
    std::vector<bool> m_modeBlockflag;
    int m_modeBits = 0;
    bool m_prevBlockflag = false;
};

} // namespace Audio