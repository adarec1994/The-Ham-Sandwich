// WwiseVorbisDecoder.cpp - Wwise Vorbis to standard Vorbis/PCM decoder
// Based on hcs64's ww2ogg and vgmstream's implementation
//
// The Wwise format stores Vorbis data in a modified way:
// - Setup headers are stripped and codebooks referenced by ID
// - Audio packets may have mode/window flags removed
// - Packet headers use custom sizes instead of Ogg pages
//
// This decoder rebuilds standard Vorbis from Wwise data.

#include "WwiseVorbisDecoder.h"
#include "WwiseCodeBooks.h"
#include <cstring>
#include <algorithm>

// Try to include stb_vorbis for PCM decoding
#if __has_include("stb_vorbis.c")
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.c"
#define HAS_STB_VORBIS 1
#else
#define HAS_STB_VORBIS 0
#endif

namespace Audio
{

//=============================================================================
// Utility functions
//=============================================================================

static inline uint16_t ReadU16LE(const uint8_t* data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static inline uint32_t ReadU32LE(const uint8_t* data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static inline int32_t ReadS32LE(const uint8_t* data)
{
    return (int32_t)ReadU32LE(data);
}

static inline uint16_t ReadU16BE(const uint8_t* data)
{
    return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

static inline uint32_t ReadU32BE(const uint8_t* data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static inline int32_t ReadS32BE(const uint8_t* data)
{
    return (int32_t)ReadU32BE(data);
}

// ilog from Tremor - integer log base 2
static int ilog(unsigned int v)
{
    int ret = 0;
    while (v) { ret++; v >>= 1; }
    return ret;
}

// Calculate quantvals for codebook lookup type 1
static unsigned int book_maptype1_quantvals(unsigned int entries, unsigned int dimensions)
{
    int bits = ilog(entries);
    int vals = entries >> ((bits - 1) * (dimensions - 1) / dimensions);

    while (1)
    {
        unsigned long acc = 1;
        unsigned long acc1 = 1;
        for (unsigned int i = 0; i < dimensions; i++)
        {
            acc *= vals;
            acc1 *= vals + 1;
        }
        if (acc <= entries && acc1 > entries)
            return vals;
        else if (acc > entries)
            vals--;
        else
            vals++;
    }
}

//=============================================================================
// Bit stream classes
//=============================================================================

class BitReader
{
public:
    BitReader(const uint8_t* data, size_t size)
        : m_data(data), m_size(size), m_bytePos(0), m_bitPos(0) {}

    uint32_t Read(int bits)
    {
        uint32_t result = 0;
        int resultBits = 0;

        while (bits > 0 && m_bytePos < m_size)
        {
            int bitsAvail = 8 - m_bitPos;
            int bitsToRead = std::min(bits, bitsAvail);
            uint8_t mask = (1 << bitsToRead) - 1;
            result |= (uint32_t)((m_data[m_bytePos] >> m_bitPos) & mask) << resultBits;

            m_bitPos += bitsToRead;
            if (m_bitPos >= 8) { m_bitPos = 0; m_bytePos++; }
            resultBits += bitsToRead;
            bits -= bitsToRead;
        }
        return result;
    }

    void Skip(int bits)
    {
        m_bitPos += bits;
        while (m_bitPos >= 8) { m_bitPos -= 8; m_bytePos++; }
    }

    size_t Position() const { return m_bytePos * 8 + m_bitPos; }
    size_t Remaining() const { return (m_size - m_bytePos) * 8 - m_bitPos; }
    bool AtEnd() const { return m_bytePos >= m_size; }

private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_bytePos;
    int m_bitPos;
};

class BitWriter
{
public:
    void Write(uint32_t value, int bits)
    {
        while (bits > 0)
        {
            if (m_bitPos == 0)
                m_data.push_back(0);

            int bitsAvail = 8 - m_bitPos;
            int bitsToWrite = std::min(bits, bitsAvail);
            uint8_t mask = (1 << bitsToWrite) - 1;
            m_data.back() |= (value & mask) << m_bitPos;

            value >>= bitsToWrite;
            m_bitPos += bitsToWrite;
            if (m_bitPos >= 8) m_bitPos = 0;
            bits -= bitsToWrite;
        }
    }

    void WriteByte(uint8_t value) { Write(value, 8); }

    void WriteBytes(const uint8_t* data, size_t count)
    {
        for (size_t i = 0; i < count; i++)
            WriteByte(data[i]);
    }

    void Flush()
    {
        if (m_bitPos > 0)
            m_bitPos = 0;
    }

    std::vector<uint8_t>& Data() { return m_data; }
    size_t BitPosition() const { return m_data.size() * 8 - (m_bitPos ? (8 - m_bitPos) : 0); }

private:
    std::vector<uint8_t> m_data;
    int m_bitPos = 0;
};

//=============================================================================
// Ogg page building
//=============================================================================

// Ogg CRC lookup table
static uint32_t g_oggCrc[256];
static bool g_oggCrcInit = false;

static void InitOggCrc()
{
    if (g_oggCrcInit) return;
    for (int i = 0; i < 256; i++)
    {
        uint32_t r = i << 24;
        for (int j = 0; j < 8; j++)
            r = (r << 1) ^ (r & 0x80000000U ? 0x04c11db7U : 0);
        g_oggCrc[i] = r;
    }
    g_oggCrcInit = true;
}

static uint32_t CalcOggCrc(const uint8_t* data, size_t size)
{
    InitOggCrc();
    uint32_t crc = 0;
    for (size_t i = 0; i < size; i++)
        crc = (crc << 8) ^ g_oggCrc[((crc >> 24) ^ data[i]) & 0xFF];
    return crc;
}

class OggBuilder
{
public:
    OggBuilder() : m_serialNo(0x12345678), m_pageNo(0) {}

    void AddPacket(const std::vector<uint8_t>& packet, bool bos, bool eos, int64_t granulePos)
    {
        std::vector<uint8_t> page;

        // OggS capture pattern
        page.push_back('O'); page.push_back('g'); page.push_back('g'); page.push_back('S');

        // Version (always 0)
        page.push_back(0);

        // Header type flags
        uint8_t flags = 0;
        if (bos) flags |= 0x02;  // Beginning of stream
        if (eos) flags |= 0x04;  // End of stream
        page.push_back(flags);

        // Granule position (8 bytes LE)
        for (int i = 0; i < 8; i++)
            page.push_back((granulePos >> (i * 8)) & 0xFF);

        // Serial number (4 bytes LE)
        for (int i = 0; i < 4; i++)
            page.push_back((m_serialNo >> (i * 8)) & 0xFF);

        // Page sequence number (4 bytes LE)
        for (int i = 0; i < 4; i++)
            page.push_back((m_pageNo >> (i * 8)) & 0xFF);
        m_pageNo++;

        // CRC placeholder (4 bytes)
        size_t crcPos = page.size();
        page.push_back(0); page.push_back(0); page.push_back(0); page.push_back(0);

        // Segment table
        size_t dataSize = packet.size();
        int numSegments = (int)((dataSize + 254) / 255);
        if (numSegments == 0) numSegments = 1;
        page.push_back((uint8_t)numSegments);

        size_t remaining = dataSize;
        for (int i = 0; i < numSegments; i++)
        {
            if (remaining >= 255) { page.push_back(255); remaining -= 255; }
            else { page.push_back((uint8_t)remaining); remaining = 0; }
        }

        // Packet data
        page.insert(page.end(), packet.begin(), packet.end());

        // Calculate CRC
        uint32_t crc = CalcOggCrc(page.data(), page.size());
        page[crcPos + 0] = (crc >> 0) & 0xFF;
        page[crcPos + 1] = (crc >> 8) & 0xFF;
        page[crcPos + 2] = (crc >> 16) & 0xFF;
        page[crcPos + 3] = (crc >> 24) & 0xFF;

        m_data.insert(m_data.end(), page.begin(), page.end());
    }

    std::vector<uint8_t>& Data() { return m_data; }

private:
    std::vector<uint8_t> m_data;
    uint32_t m_serialNo;
    uint32_t m_pageNo;
};

//=============================================================================
// WwiseVorbisDecoder implementation
//=============================================================================

WwiseVorbisDecoder::WwiseVorbisDecoder() = default;
WwiseVorbisDecoder::~WwiseVorbisDecoder() = default;

bool WwiseVorbisDecoder::ParseWEM(const uint8_t* data, size_t size)
{
    m_rawData.assign(data, data + size);
    m_lastError.clear();

    if (size < 12)
    {
        m_lastError = "File too small";
        return false;
    }

    // Check RIFF/RIFX header
    bool isRIFF = memcmp(data, "RIFF", 4) == 0;
    bool isRIFX = memcmp(data, "RIFX", 4) == 0;

    if (!isRIFF && !isRIFX)
    {
        m_lastError = "Not a RIFF/RIFX file";
        return false;
    }

    m_config.bigEndian = isRIFX;

    auto ReadU16 = m_config.bigEndian ? ReadU16BE : ReadU16LE;
    auto ReadU32 = m_config.bigEndian ? ReadU32BE : ReadU32LE;
    auto ReadS32 = m_config.bigEndian ? ReadS32BE : ReadS32LE;

    if (memcmp(data + 8, "WAVE", 4) != 0)
    {
        m_lastError = "Not a WAVE file";
        return false;
    }

    // Parse RIFF chunks
    size_t offset = 12;
    bool hasFmt = false;
    bool hasData = false;

    uint32_t vorbOffset = 0, vorbSize = 0;

    while (offset + 8 <= size)
    {
        char chunkId[5] = {0};
        memcpy(chunkId, data + offset, 4);
        uint32_t chunkSize = ReadU32(data + offset + 4);

        if (offset + 8 + chunkSize > size)
            break;

        const uint8_t* chunkData = data + offset + 8;

        if (memcmp(chunkId, "fmt ", 4) == 0)
        {
            hasFmt = true;

            uint16_t formatTag = ReadU16(chunkData);
            m_config.channels = ReadU16(chunkData + 2);
            m_config.sampleRate = ReadU32(chunkData + 4);

            if (formatTag != 0xFFFF)
            {
                m_lastError = "Not Wwise Vorbis format (tag=" + std::to_string(formatTag) + ")";
                return false;
            }

            // Determine format version from fmt chunk size
            if (chunkSize == 0x18) // 24 bytes - oldest
            {
                m_config.headerType = WwiseHeaderType::Type8;
                m_config.setupType = WwiseSetupType::HeaderTriad;
                m_config.packetType = WwisePacketType::Standard;
            }
            else if (chunkSize == 0x28) // 40 bytes - early
            {
                m_config.headerType = WwiseHeaderType::Type6;
                m_config.packetType = WwisePacketType::Standard;
            }
            else if (chunkSize == 0x18 + 0x30 || chunkSize >= 0x42) // with vorb inline
            {
                // Modern format with inline vorb data
                m_config.headerType = WwiseHeaderType::Type2;
                m_config.packetType = WwisePacketType::Modified;
                m_config.setupType = WwiseSetupType::AoTuV603Codebooks;

                // Parse inline vorb-style data (after cbSize field at offset 16)
                if (chunkSize >= 0x42)
                {
                    uint16_t cbSize = ReadU16(chunkData + 16);
                    const uint8_t* extra = chunkData + 18;

                    m_config.sampleCount = ReadU32(extra + 0);
                    // extra+4 = mod signal
                    // extra+8 = setup ID (sometimes)
                    m_config.setupOffset = ReadU32(extra + 16);
                    m_config.firstAudioOffset = ReadU32(extra + 20);

                    // Blocksizes at specific offsets
                    if (chunkSize >= 0x48)
                    {
                        m_config.blocksize0Exp = chunkData[0x3C];
                        m_config.blocksize1Exp = chunkData[0x3E];
                    }
                }
            }
        }
        else if (memcmp(chunkId, "vorb", 4) == 0)
        {
            // Separate vorb chunk (older files)
            vorbOffset = offset + 8;
            vorbSize = chunkSize;

            if (chunkSize >= 0x28)
            {
                m_config.sampleCount = ReadU32(chunkData + 0);
                m_config.setupOffset = ReadU32(chunkData + 4);
                m_config.firstAudioOffset = ReadU32(chunkData + 8);

                // Detect version from vorb size
                if (chunkSize == 0x2A)
                {
                    m_config.headerType = WwiseHeaderType::Type6;
                    m_config.packetType = WwisePacketType::Standard;
                    m_config.blocksize0Exp = chunkData[0x24];
                    m_config.blocksize1Exp = chunkData[0x26];
                }
                else if (chunkSize == 0x34 || chunkSize == 0x32)
                {
                    m_config.headerType = WwiseHeaderType::Type2;
                    m_config.packetType = WwisePacketType::Modified;
                    m_config.blocksize0Exp = chunkData[0x28];
                    m_config.blocksize1Exp = chunkData[0x2A];
                }
            }
        }
        else if (memcmp(chunkId, "data", 4) == 0)
        {
            hasData = true;
            m_config.dataOffset = offset + 8;
            m_config.dataSize = chunkSize;
        }

        offset += 8 + chunkSize;
        if (chunkSize & 1) offset++; // Padding
    }

    if (!hasFmt)
    {
        m_lastError = "Missing fmt chunk";
        return false;
    }

    if (!hasData)
    {
        m_lastError = "Missing data chunk";
        return false;
    }

    // Default blocksizes if not found
    if (m_config.blocksize0Exp == 0) m_config.blocksize0Exp = 8;
    if (m_config.blocksize1Exp == 0) m_config.blocksize1Exp = 11;

    // Detect if we need modified packet handling
    if (m_config.blocksize0Exp == m_config.blocksize1Exp)
        m_config.packetType = WwisePacketType::Standard;

    return true;
}

std::vector<uint8_t> WwiseVorbisDecoder::BuildIdentificationHeader()
{
    std::vector<uint8_t> header;

    // Packet type (1 = identification)
    header.push_back(0x01);

    // "vorbis" signature
    header.push_back('v'); header.push_back('o'); header.push_back('r');
    header.push_back('b'); header.push_back('i'); header.push_back('s');

    // Vorbis version (always 0)
    header.push_back(0); header.push_back(0); header.push_back(0); header.push_back(0);

    // Channels
    header.push_back((uint8_t)m_config.channels);

    // Sample rate (4 bytes LE)
    header.push_back((m_config.sampleRate >> 0) & 0xFF);
    header.push_back((m_config.sampleRate >> 8) & 0xFF);
    header.push_back((m_config.sampleRate >> 16) & 0xFF);
    header.push_back((m_config.sampleRate >> 24) & 0xFF);

    // Bitrate maximum (0 = unset)
    header.push_back(0); header.push_back(0); header.push_back(0); header.push_back(0);

    // Bitrate nominal (0 = unset)
    header.push_back(0); header.push_back(0); header.push_back(0); header.push_back(0);

    // Bitrate minimum (0 = unset)
    header.push_back(0); header.push_back(0); header.push_back(0); header.push_back(0);

    // Block sizes (4 bits each, packed)
    uint8_t blocksizes = (m_config.blocksize0Exp & 0x0F) | ((m_config.blocksize1Exp & 0x0F) << 4);
    header.push_back(blocksizes);

    // Framing flag
    header.push_back(0x01);

    return header;
}

std::vector<uint8_t> WwiseVorbisDecoder::BuildCommentHeader()
{
    std::vector<uint8_t> header;

    // Packet type (3 = comment)
    header.push_back(0x03);

    // "vorbis" signature
    header.push_back('v'); header.push_back('o'); header.push_back('r');
    header.push_back('b'); header.push_back('i'); header.push_back('s');

    // Vendor string length (empty)
    header.push_back(0); header.push_back(0); header.push_back(0); header.push_back(0);

    // User comment list length (0 comments)
    header.push_back(0); header.push_back(0); header.push_back(0); header.push_back(0);

    // Framing flag
    header.push_back(0x01);

    return header;
}

// Rebuild a Wwise codebook to standard Vorbis format
bool WwiseVorbisDecoder::RebuildCodebook(uint32_t codebookId, std::vector<uint8_t>& output)
{
    size_t cbSize = 0;
    const uint8_t* cbData = GetCodebookData(codebookId, cbSize);

    if (!cbData || cbSize == 0)
    {
        m_lastError = "Codebook " + std::to_string(codebookId) + " not found";
        return false;
    }

    BitReader br(cbData, cbSize);
    BitWriter bw;

    // Write VCB sync pattern
    bw.Write(0x564342, 24); // "VCB" backwards due to bit ordering

    // Read dimensions (4 bits in Wwise, 16 bits in Vorbis)
    uint32_t dimensions = br.Read(4);
    bw.Write(dimensions, 16);

    // Read entries (14 bits in Wwise, 24 bits in Vorbis)
    uint32_t entries = br.Read(14);
    bw.Write(entries, 24);

    // Codeword lengths
    uint32_t ordered = br.Read(1);
    bw.Write(ordered, 1);

    if (ordered)
    {
        uint32_t initialLength = br.Read(5);
        bw.Write(initialLength, 5);

        uint32_t currentEntry = 0;
        while (currentEntry < entries)
        {
            int numberBits = ilog(entries - currentEntry);
            uint32_t number = br.Read(numberBits);
            bw.Write(number, numberBits);
            currentEntry += number;
        }
    }
    else
    {
        uint32_t codewordLengthLength = br.Read(3);
        uint32_t sparse = br.Read(1);
        bw.Write(sparse, 1);

        if (codewordLengthLength == 0 || codewordLengthLength > 5)
        {
            m_lastError = "Invalid codeword length length";
            return false;
        }

        for (uint32_t i = 0; i < entries; i++)
        {
            bool present = true;
            if (sparse)
            {
                present = br.Read(1) != 0;
                bw.Write(present ? 1 : 0, 1);
            }

            if (present)
            {
                uint32_t codewordLength = br.Read(codewordLengthLength);
                bw.Write(codewordLength, 5); // Always 5 bits in output
            }
        }
    }

    // Lookup table
    uint32_t lookupType = br.Read(1);
    bw.Write(lookupType, 4); // 1 bit in Wwise, 4 bits in Vorbis

    if (lookupType == 1)
    {
        uint32_t minValue = br.Read(32);
        bw.Write(minValue, 32);

        uint32_t maxValue = br.Read(32);
        bw.Write(maxValue, 32);

        uint32_t valueLength = br.Read(4);
        bw.Write(valueLength, 4);

        uint32_t sequenceFlag = br.Read(1);
        bw.Write(sequenceFlag, 1);

        uint32_t quantvals = book_maptype1_quantvals(entries, dimensions);
        for (uint32_t i = 0; i < quantvals; i++)
        {
            uint32_t val = br.Read(valueLength + 1);
            bw.Write(val, valueLength + 1);
        }
    }

    bw.Flush();
    output = std::move(bw.Data());
    return true;
}

std::vector<uint8_t> WwiseVorbisDecoder::BuildSetupHeader()
{
    // Read setup packet from Wwise data
    const uint8_t* setupData = m_rawData.data() + m_config.dataOffset + m_config.setupOffset;
    size_t remaining = m_config.dataSize - m_config.setupOffset;

    // Read packet header to get size
    uint16_t packetSize = 0;
    size_t headerSize = 0;

    auto ReadU16 = m_config.bigEndian ? ReadU16BE : ReadU16LE;
    auto ReadU32 = m_config.bigEndian ? ReadU32BE : ReadU32LE;

    switch (m_config.headerType)
    {
        case WwiseHeaderType::Type8:
            packetSize = (uint16_t)ReadU32(setupData);
            headerSize = 8;
            break;
        case WwiseHeaderType::Type6:
            packetSize = ReadU16(setupData);
            headerSize = 6;
            break;
        case WwiseHeaderType::Type2:
        default:
            packetSize = ReadU16(setupData);
            headerSize = 2;
            break;
    }

    if (packetSize == 0 || headerSize + packetSize > remaining)
    {
        m_lastError = "Invalid setup packet size";
        return {};
    }

    BitReader br(setupData + headerSize, packetSize);
    BitWriter bw;

    // Write setup packet header
    bw.Write(0x05, 8);  // Packet type = setup
    bw.WriteByte('v'); bw.WriteByte('o'); bw.WriteByte('r');
    bw.WriteByte('b'); bw.WriteByte('i'); bw.WriteByte('s');

    // Codebook count
    uint32_t codebookCount = br.Read(8) + 1;
    bw.Write(codebookCount - 1, 8);

    // Rebuild each codebook
    for (uint32_t i = 0; i < codebookCount; i++)
    {
        if (m_config.setupType == WwiseSetupType::ExternalCodebooks ||
            m_config.setupType == WwiseSetupType::AoTuV603Codebooks)
        {
            // External codebook - read 10-bit ID
            uint32_t codebookId = br.Read(10);

            std::vector<uint8_t> cbData;
            if (!RebuildCodebook(codebookId, cbData))
                return {};

            // Copy codebook data bit by bit
            BitReader cbReader(cbData.data(), cbData.size());
            while (cbReader.Remaining() >= 8)
                bw.Write(cbReader.Read(8), 8);
            int leftover = cbReader.Remaining();
            if (leftover > 0)
                bw.Write(cbReader.Read(leftover), leftover);
        }
        else
        {
            m_lastError = "Unsupported setup type";
            return {};
        }
    }

    // Time domain transforms (dummy)
    bw.Write(0, 6);     // count - 1 = 0
    bw.Write(0, 16);    // dummy value

    // Floors
    uint32_t floorCount = br.Read(6) + 1;
    bw.Write(floorCount - 1, 6);

    for (uint32_t i = 0; i < floorCount; i++)
    {
        bw.Write(1, 16);  // Floor type 1

        uint32_t partitions = br.Read(5);
        bw.Write(partitions, 5);

        uint32_t maxClass = 0;
        std::vector<uint32_t> partitionClassList(partitions);

        for (uint32_t j = 0; j < partitions; j++)
        {
            uint32_t partitionClass = br.Read(4);
            bw.Write(partitionClass, 4);
            partitionClassList[j] = partitionClass;
            if (partitionClass > maxClass)
                maxClass = partitionClass;
        }

        std::vector<uint32_t> classDimensions(maxClass + 1);

        for (uint32_t j = 0; j <= maxClass; j++)
        {
            uint32_t dimensions = br.Read(3) + 1;
            bw.Write(dimensions - 1, 3);
            classDimensions[j] = dimensions;

            uint32_t subclasses = br.Read(2);
            bw.Write(subclasses, 2);

            if (subclasses != 0)
            {
                uint32_t masterbook = br.Read(8);
                bw.Write(masterbook, 8);
            }

            for (uint32_t k = 0; k < (1u << subclasses); k++)
            {
                uint32_t subclassBook = br.Read(8);
                bw.Write(subclassBook, 8);
            }
        }

        uint32_t multiplier = br.Read(2);
        bw.Write(multiplier, 2);

        uint32_t rangebits = br.Read(4);
        bw.Write(rangebits, 4);

        for (uint32_t j = 0; j < partitions; j++)
        {
            uint32_t classNum = partitionClassList[j];
            for (uint32_t k = 0; k < classDimensions[classNum]; k++)
            {
                uint32_t x = br.Read(rangebits);
                bw.Write(x, rangebits);
            }
        }
    }

    // Residues
    uint32_t residueCount = br.Read(6) + 1;
    bw.Write(residueCount - 1, 6);

    for (uint32_t i = 0; i < residueCount; i++)
    {
        uint32_t residueType = br.Read(2);
        bw.Write(residueType, 16);  // 2 bits to 16 bits

        bw.Write(br.Read(24), 24);  // begin
        bw.Write(br.Read(24), 24);  // end
        bw.Write(br.Read(24), 24);  // partition size

        uint32_t classifications = br.Read(6) + 1;
        bw.Write(classifications - 1, 6);

        uint32_t classbook = br.Read(8);
        bw.Write(classbook, 8);

        std::vector<uint32_t> cascade(classifications);
        for (uint32_t j = 0; j < classifications; j++)
        {
            uint32_t lowBits = br.Read(3);
            bw.Write(lowBits, 3);

            uint32_t bitflag = br.Read(1);
            bw.Write(bitflag, 1);

            uint32_t highBits = 0;
            if (bitflag)
            {
                highBits = br.Read(5);
                bw.Write(highBits, 5);
            }

            cascade[j] = highBits * 8 + lowBits;
        }

        for (uint32_t j = 0; j < classifications; j++)
        {
            for (int k = 0; k < 8; k++)
            {
                if (cascade[j] & (1 << k))
                {
                    uint32_t book = br.Read(8);
                    bw.Write(book, 8);
                }
            }
        }
    }

    // Mappings
    uint32_t mappingCount = br.Read(6) + 1;
    bw.Write(mappingCount - 1, 6);

    for (uint32_t i = 0; i < mappingCount; i++)
    {
        bw.Write(0, 16);  // Mapping type 0

        uint32_t submapsFlag = br.Read(1);
        bw.Write(submapsFlag, 1);

        uint32_t submaps = 1;
        if (submapsFlag)
        {
            submaps = br.Read(4) + 1;
            bw.Write(submaps - 1, 4);
        }

        uint32_t couplingFlag = br.Read(1);
        bw.Write(couplingFlag, 1);

        if (couplingFlag)
        {
            uint32_t couplingSteps = br.Read(8) + 1;
            bw.Write(couplingSteps - 1, 8);

            int couplingBits = ilog(m_config.channels - 1);
            for (uint32_t j = 0; j < couplingSteps; j++)
            {
                bw.Write(br.Read(couplingBits), couplingBits);  // magnitude
                bw.Write(br.Read(couplingBits), couplingBits);  // angle
            }
        }

        uint32_t reserved = br.Read(2);
        bw.Write(reserved, 2);

        if (submaps > 1)
        {
            for (int j = 0; j < m_config.channels; j++)
            {
                uint32_t mux = br.Read(4);
                bw.Write(mux, 4);
            }
        }

        for (uint32_t j = 0; j < submaps; j++)
        {
            bw.Write(br.Read(8), 8);  // time config (unused)
            bw.Write(br.Read(8), 8);  // floor number
            bw.Write(br.Read(8), 8);  // residue number
        }
    }

    // Modes
    uint32_t modeCount = br.Read(6) + 1;
    bw.Write(modeCount - 1, 6);

    m_modeBlockflag.resize(modeCount);
    m_modeBits = ilog(modeCount - 1);

    for (uint32_t i = 0; i < modeCount; i++)
    {
        uint32_t blockflag = br.Read(1);
        bw.Write(blockflag, 1);
        m_modeBlockflag[i] = (blockflag != 0);

        bw.Write(0, 16);  // windowtype = 0
        bw.Write(0, 16);  // transformtype = 0

        uint32_t mapping = br.Read(8);
        bw.Write(mapping, 8);
    }

    // Framing flag
    bw.Write(1, 1);
    bw.Flush();

    return bw.Data();
}

bool WwiseVorbisDecoder::ReadPacketHeader(size_t offset, uint16_t& packetSize, int32_t& granulePos)
{
    if (offset >= m_rawData.size())
        return false;

    const uint8_t* data = m_rawData.data() + offset;
    size_t remaining = m_rawData.size() - offset;

    auto ReadU16 = m_config.bigEndian ? ReadU16BE : ReadU16LE;
    auto ReadU32 = m_config.bigEndian ? ReadU32BE : ReadU32LE;
    auto ReadS32 = m_config.bigEndian ? ReadS32BE : ReadS32LE;

    switch (m_config.headerType)
    {
        case WwiseHeaderType::Type8:
            if (remaining < 8) return false;
            packetSize = (uint16_t)ReadU32(data);
            granulePos = ReadS32(data + 4);
            return true;

        case WwiseHeaderType::Type6:
            if (remaining < 6) return false;
            packetSize = ReadU16(data);
            granulePos = ReadS32(data + 2);
            return true;

        case WwiseHeaderType::Type2:
        default:
            if (remaining < 2) return false;
            packetSize = ReadU16(data);
            granulePos = 0;
            return true;
    }
}

bool WwiseVorbisDecoder::ConvertAudioPacket(const uint8_t* input, size_t inputSize,
                                            std::vector<uint8_t>& output,
                                            bool hasNext, uint8_t nextByte)
{
    if (inputSize == 0)
        return false;

    if (m_config.packetType == WwisePacketType::Standard)
    {
        // Standard packets - just copy
        output.assign(input, input + inputSize);
        return true;
    }

    // Modified packets - need to rebuild mode/window info
    BitReader br(input, inputSize);
    BitWriter bw;

    // Audio packet type (0)
    bw.Write(0, 1);

    // Mode number
    uint32_t modeNumber = br.Read(m_modeBits);
    bw.Write(modeNumber, m_modeBits);

    // Read remainder of first byte
    int remainderBits = 8 - m_modeBits;
    uint32_t remainder = br.Read(remainderBits);

    // For long blocks, add window flags
    if (modeNumber < m_modeBlockflag.size() && m_modeBlockflag[modeNumber])
    {
        // Previous window flag
        bw.Write(m_prevBlockflag ? 1 : 0, 1);

        // Next window flag - peek at next packet's mode
        bool nextBlockflag = false;
        if (hasNext && m_modeBits > 0)
        {
            uint32_t nextMode = nextByte & ((1 << m_modeBits) - 1);
            if (nextMode < m_modeBlockflag.size())
                nextBlockflag = m_modeBlockflag[nextMode];
        }
        bw.Write(nextBlockflag ? 1 : 0, 1);
    }

    // Save blockflag for next packet
    if (modeNumber < m_modeBlockflag.size())
        m_prevBlockflag = m_modeBlockflag[modeNumber];

    // Write remainder bits
    bw.Write(remainder, remainderBits);

    // Copy rest of packet
    while (br.Remaining() >= 8)
        bw.Write(br.Read(8), 8);

    int leftover = br.Remaining();
    if (leftover > 0)
        bw.Write(br.Read(leftover), leftover);

    bw.Flush();
    output = std::move(bw.Data());
    return true;
}

bool WwiseVorbisDecoder::ConvertToOgg(std::vector<uint8_t>& outOgg)
{
    if (m_rawData.empty())
    {
        m_lastError = "No data loaded";
        return false;
    }

    // Check codebooks are loaded
    if (!IsCodebooksInitialized() &&
        (m_config.setupType == WwiseSetupType::ExternalCodebooks ||
         m_config.setupType == WwiseSetupType::AoTuV603Codebooks))
    {
        m_lastError = "Codebooks not initialized - call InitializeCodebooks() first";
        return false;
    }

    // Build headers
    auto idHeader = BuildIdentificationHeader();
    auto commentHeader = BuildCommentHeader();
    auto setupHeader = BuildSetupHeader();

    if (setupHeader.empty())
        return false;

    // Build OGG
    OggBuilder ogg;

    ogg.AddPacket(idHeader, true, false, 0);
    ogg.AddPacket(commentHeader, false, false, 0);
    ogg.AddPacket(setupHeader, false, false, 0);

    // Convert audio packets
    size_t headerSize = 0;
    switch (m_config.headerType)
    {
        case WwiseHeaderType::Type8: headerSize = 8; break;
        case WwiseHeaderType::Type6: headerSize = 6; break;
        case WwiseHeaderType::Type2: headerSize = 2; break;
    }

    size_t offset = m_config.dataOffset + m_config.firstAudioOffset;
    size_t endOffset = m_config.dataOffset + m_config.dataSize;
    int64_t granulePos = 0;
    int blocksize0 = 1 << m_config.blocksize0Exp;
    int blocksize1 = 1 << m_config.blocksize1Exp;

    m_prevBlockflag = false;

    while (offset + headerSize < endOffset)
    {
        uint16_t packetSize = 0;
        int32_t packetGranule = 0;

        if (!ReadPacketHeader(offset, packetSize, packetGranule))
            break;

        if (packetSize == 0 || packetSize == 0xFFFF)
            break;

        if (offset + headerSize + packetSize > endOffset)
            break;

        const uint8_t* packetData = m_rawData.data() + offset + headerSize;

        // Check for next packet's first byte (for modified packets)
        bool hasNext = false;
        uint8_t nextByte = 0;

        size_t nextOffset = offset + headerSize + packetSize;
        if (nextOffset + headerSize + 1 <= endOffset)
        {
            uint16_t nextSize = 0;
            int32_t nextGranule = 0;
            if (ReadPacketHeader(nextOffset, nextSize, nextGranule) && nextSize > 0)
            {
                hasNext = true;
                nextByte = m_rawData[nextOffset + headerSize];
            }
        }

        std::vector<uint8_t> vorbisPacket;
        if (!ConvertAudioPacket(packetData, packetSize, vorbisPacket, hasNext, nextByte))
        {
            offset += headerSize + packetSize;
            continue;
        }

        // Update granule position
        granulePos += blocksize0 / 4;  // Approximate

        bool isLast = (nextOffset + headerSize >= endOffset);
        ogg.AddPacket(vorbisPacket, false, isLast, granulePos);

        offset = nextOffset;
    }

    outOgg = std::move(ogg.Data());
    return true;
}

int WwiseVorbisDecoder::DecodeToPCM(std::vector<int16_t>& outSamples)
{
    outSamples.clear();

#if HAS_STB_VORBIS
    std::vector<uint8_t> oggData;
    if (!ConvertToOgg(oggData))
        return -1;

    int channels = 0;
    int sampleRate = 0;
    short* output = nullptr;

    int samples = stb_vorbis_decode_memory(
        oggData.data(), (int)oggData.size(),
        &channels, &sampleRate, &output);

    if (samples <= 0 || !output)
    {
        m_lastError = "stb_vorbis decode failed";
        return -1;
    }

    outSamples.assign(output, output + samples * channels);
    free(output);

    return samples;
#else
    m_lastError = "stb_vorbis not available - use ConvertToOgg() and an external decoder";
    return -1;
#endif
}

} // namespace Audio