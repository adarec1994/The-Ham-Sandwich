// WwiseVorbisDecoder.cpp - Wwise Vorbis to OGG converter
// Based on vgmstream's ww2ogg implementation by hcs
// Fixed implementation

#include "WwiseVorbisDecoder.h"
#include "WwiseCodebookData.h"
#include <cstring>
#include <algorithm>

namespace Audio {

// Little-endian readers
static inline uint16_t ReadU16LE(const uint8_t* p) { return p[0] | (p[1] << 8); }
static inline uint32_t ReadU32LE(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static inline uint32_t ReadU32BE(const uint8_t* p) { return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

//=============================================================================
// Vorbis Bitstream Reader (LSB first, Vorbis packing)
//=============================================================================
class BitReader {
public:
    BitReader(const uint8_t* data, size_t size) : m_data(data), m_size(size), m_bitPos(0) {}

    uint32_t Read(int bits) {
        uint32_t result = 0;
        for (int i = 0; i < bits; i++) {
            size_t bytePos = m_bitPos / 8;
            int bitInByte = m_bitPos % 8;
            if (bytePos >= m_size) return result;
            uint32_t bit = (m_data[bytePos] >> bitInByte) & 1;
            result |= bit << i;
            m_bitPos++;
        }
        return result;
    }

    size_t TotalBits() const { return m_size * 8; }
    size_t BitsRead() const { return m_bitPos; }
    size_t BitsLeft() const { return m_size * 8 - m_bitPos; }
    bool AtEnd() const { return m_bitPos >= m_size * 8; }
    size_t BytePosition() const { return (m_bitPos + 7) / 8; }

private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_bitPos;
};

//=============================================================================
// Vorbis Bitstream Writer (LSB first, Vorbis packing)
//=============================================================================
class BitWriter {
public:
    BitWriter() : m_bitPos(0) {}

    void Write(uint32_t value, int bits) {
        for (int i = 0; i < bits; i++) {
            size_t bytePos = m_bitPos / 8;
            int bitInByte = m_bitPos % 8;

            while (bytePos >= m_buffer.size()) {
                m_buffer.push_back(0);
            }

            if (value & (1u << i)) {
                m_buffer[bytePos] |= (1 << bitInByte);
            }
            m_bitPos++;
        }
    }

    void WriteBytes(const uint8_t* data, size_t size) {
        FlushToByte();
        m_buffer.insert(m_buffer.end(), data, data + size);
        m_bitPos = m_buffer.size() * 8;
    }

    void FlushToByte() {
        if (m_bitPos % 8 != 0) {
            m_bitPos = ((m_bitPos + 7) / 8) * 8;
        }
        size_t neededBytes = (m_bitPos + 7) / 8;
        while (m_buffer.size() < neededBytes) {
            m_buffer.push_back(0);
        }
    }

    const std::vector<uint8_t>& Data() const { return m_buffer; }
    std::vector<uint8_t>& Data() { return m_buffer; }
    size_t BitPosition() const { return m_bitPos; }

private:
    std::vector<uint8_t> m_buffer;
    size_t m_bitPos;
};

//=============================================================================
// Tremor helper functions (from Xiph's fixed-point Vorbis)
//=============================================================================
static int ilog(unsigned int v) {
    int ret = 0;
    while (v) { ret++; v >>= 1; }
    return ret;
}

static unsigned int book_maptype1_quantvals(unsigned int entries, unsigned int dimensions) {
    if (dimensions == 0) return 0;
    int bits = ilog(entries);
    int vals = entries >> ((bits - 1) * (dimensions - 1) / dimensions);

    while (1) {
        unsigned long acc = 1, acc1 = 1;
        for (unsigned int i = 0; i < dimensions; i++) {
            acc *= vals;
            acc1 *= vals + 1;
        }
        if (acc <= entries && acc1 > entries) return vals;
        else if (acc > entries) vals--;
        else vals++;
    }
}

//=============================================================================
// Rebuild Wwise codebook to Vorbis format
// This converts from Wwise's compressed format to standard Vorbis codebook format
//=============================================================================
static bool RebuildCodebook(BitReader& iw, BitWriter& ow) {
    // Write "VCB" sync pattern (0x564342)
    // In LSB bit packing: writes 0x42='B', 0x43='C', 0x56='V' in order
    ow.Write(0x564342, 24);

    // Dimensions: 4 bits in Wwise -> 16 bits in Vorbis
    uint32_t dimensions = iw.Read(4);
    ow.Write(dimensions, 16);

    // Entries: 14 bits in Wwise -> 24 bits in Vorbis
    uint32_t entries = iw.Read(14);
    ow.Write(entries, 24);

    // Ordered flag
    uint32_t ordered = iw.Read(1);
    ow.Write(ordered, 1);

    if (ordered) {
        uint32_t initialLength = iw.Read(5);
        ow.Write(initialLength, 5);

        uint32_t currentEntry = 0;
        while (currentEntry < entries) {
            int numberBits = ilog(entries - currentEntry);
            uint32_t number = iw.Read(numberBits);
            ow.Write(number, numberBits);
            currentEntry += number;
        }
        if (currentEntry > entries) {
            return false;
        }
    }
    else {
        // Wwise stores codeword_length_length (3 bits) and sparse flag (1 bit)
        uint32_t codewordLengthLength = iw.Read(3);
        uint32_t sparse = iw.Read(1);
        ow.Write(sparse, 1);

        if (codewordLengthLength == 0 || codewordLengthLength > 5) {
            return false;
        }

        for (uint32_t i = 0; i < entries; i++) {
            bool presentBool = true;
            if (sparse) {
                uint32_t present = iw.Read(1);
                ow.Write(present, 1);
                presentBool = (present != 0);
            }

            if (presentBool) {
                uint32_t codewordLength = iw.Read(codewordLengthLength);
                // Wwise uses variable bits, Vorbis always uses 5 bits
                ow.Write(codewordLength, 5);
            }
        }
    }

    // Lookup type: 1 bit in Wwise -> 4 bits in Vorbis
    uint32_t lookupType = iw.Read(1);
    ow.Write(lookupType, 4);

    if (lookupType == 0) {
        // No lookup table
    }
    else if (lookupType == 1) {
        uint32_t minVal = iw.Read(32);
        ow.Write(minVal, 32);
        uint32_t maxVal = iw.Read(32);
        ow.Write(maxVal, 32);
        uint32_t valueLength = iw.Read(4);
        ow.Write(valueLength, 4);
        uint32_t sequenceFlag = iw.Read(1);
        ow.Write(sequenceFlag, 1);

        uint32_t quantvals = book_maptype1_quantvals(entries, dimensions);
        for (uint32_t i = 0; i < quantvals; i++) {
            uint32_t val = iw.Read(valueLength + 1);
            ow.Write(val, valueLength + 1);
        }
    }
    else {
        return false;
    }

    return true;
}

//=============================================================================
// Get codebook by ID and rebuild from Wwise format to Vorbis format
// KEY FIX: The 10-bit value is the full codebook ID
// List selection is based on setup type, not a per-codebook flag
//=============================================================================
static bool GetRebuiltCodebook(uint32_t codebookId, WwiseSetupType setupType, BitWriter& ow) {
    // Select list based on setup type
    const wvc_info* list;
    int listSize;

    if (setupType == WwiseSetupType::AoTuV603Codebooks) {
        list = wvc_list_aotuv603;
        listSize = sizeof(wvc_list_aotuv603) / sizeof(wvc_info);
    } else {
        list = wvc_list_standard;
        listSize = sizeof(wvc_list_standard) / sizeof(wvc_info);
    }

    const uint8_t* cbData = nullptr;
    size_t cbSize = 0;

    for (int i = 0; i < listSize; i++) {
        if (list[i].id == codebookId) {
            cbData = list[i].codebook;
            cbSize = list[i].size;
            break;
        }
    }

    // If not found in primary list, try the other list as fallback
    if (!cbData || cbSize == 0) {
        const wvc_info* fallbackList;
        int fallbackSize;

        if (setupType == WwiseSetupType::AoTuV603Codebooks) {
            fallbackList = wvc_list_standard;
            fallbackSize = sizeof(wvc_list_standard) / sizeof(wvc_info);
        } else {
            fallbackList = wvc_list_aotuv603;
            fallbackSize = sizeof(wvc_list_aotuv603) / sizeof(wvc_info);
        }

        for (int i = 0; i < fallbackSize; i++) {
            if (fallbackList[i].id == codebookId) {
                cbData = fallbackList[i].codebook;
                cbSize = fallbackList[i].size;
                break;
            }
        }
    }

    if (!cbData || cbSize == 0) {
        return false;
    }

    // Rebuild from Wwise packed format to standard Vorbis format
    BitReader iw(cbData, cbSize);
    return RebuildCodebook(iw, ow);
}

//=============================================================================
// OGG CRC32 (polynomial 0x04c11db7)
//=============================================================================
static uint32_t OggCRC32(const uint8_t* data, size_t size) {
    static const uint32_t crc_lookup[256] = {
        0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
        0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
        0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
        0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
        0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
        0xbe2b5158,0xbaea4cef,0xb7a96a36,0xb3687d81,0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
        0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
        0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
        0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
        0x128e97cf,0x164f8a78,0x1b0caca1,0x1fcdb116,0x018ae113,0x054bfca4,0x0808da7d,0x0cc9c7ca,
        0x7897a107,0x7c56bcb0,0x71159a69,0x75d487de,0x6b93d7db,0x6f52ca6c,0x6211ecb5,0x66d0f102,
        0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
        0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
        0x8aad212f,0x8e6c3c98,0x832f1a41,0x87ee07f6,0x99a957f3,0x9d684a44,0x902b6c9d,0x94ea712a,
        0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
        0xc6bcfa5f,0xc27de7e8,0xcf3ec131,0xcbffdc86,0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
        0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
        0x4f040756,0x4bc51ae1,0x46863c38,0x4247218f,0x5c00718a,0x58c16c3d,0x55824ae4,0x51435753,
        0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
        0x0315dc26,0x07d4c191,0x0a97e748,0x0e56faff,0x1011aafa,0x14d0b74d,0x19939194,0x1d528c23,
        0xf12f5c0e,0xf5ee41b9,0xf8ad6760,0xfc6c7ad7,0xe22b2ad2,0xe6ea3765,0xeba911bc,0xef680c0b,
        0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
        0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
        0x9b366ac6,0x9ff77771,0x92b451a8,0x96754c1f,0x88321c1a,0x8cf301ad,0x81b02774,0x85713ac3,
        0xcf48ea49,0xcb89f7fe,0xc6cad127,0xc20bcc90,0xdc4c9c95,0xd88d8122,0xd5cea7fb,0xd10fba4c,
        0xe9400df1,0xed811046,0xe0c2369f,0xe4032b28,0xfa447b2d,0xfe85669a,0xf3c64043,0xf7075df4,
        0x83593b39,0x8798268e,0x8adb0057,0x8e1a1de0,0x905d4de5,0x949c5052,0x99df768b,0x9d1e6b3c,
        0xa551dc81,0xa190c136,0xacd3e7ef,0xa812fa58,0xb655aa5d,0xb294b7ea,0xbfd79133,0xbb168c84,
        0x576b5ca9,0x53aa411e,0x5ee967c7,0x5a287a70,0x446f2a75,0x40ae37c2,0x4ded111b,0x492c0cac,
        0x7163bb11,0x75a2a6a6,0x78e1807f,0x7c209dc8,0x6267cdcd,0x66a6d07a,0x6be5f6a3,0x6f24eb14,
        0x1b7a8dd9,0x1fbb906e,0x12f8b6b7,0x1639ab00,0x087efb05,0x0cbfe6b2,0x01fcc06b,0x053ddddc,
        0x3d726a61,0x39b377d6,0x34f0510f,0x30314cb8,0x2e761cbd,0x2ab7010a,0x27f427d3,0x23353a64
    };

    uint32_t crc = 0;
    for (size_t i = 0; i < size; i++) {
        crc = (crc << 8) ^ crc_lookup[((crc >> 24) & 0xFF) ^ data[i]];
    }
    return crc;
}

//=============================================================================
// WEM Parsing
//=============================================================================
bool WwiseVorbisDecoder::ParseWEM(const uint8_t* data, size_t size) {
    m_data = data;
    m_size = size;
    m_lastError.clear();

    if (size < 12) {
        m_lastError = "File too small";
        return false;
    }

    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        m_lastError = "Not a RIFF WAVE file";
        return false;
    }

    size_t offset = 12;
    bool foundFmt = false, foundData = false;

    while (offset + 8 <= size) {
        uint32_t chunkId = ReadU32BE(data + offset);
        uint32_t chunkSize = ReadU32LE(data + offset + 4);

        if (chunkId == 0x666D7420) { // "fmt "
            if (!ParseFmtChunk(data + offset + 8, chunkSize)) return false;
            foundFmt = true;
        }
        else if (chunkId == 0x64617461) { // "data"
            m_config.dataOffset = offset + 8;
            m_config.dataSize = chunkSize;
            foundData = true;
        }

        offset += 8 + chunkSize;
        if (chunkSize & 1) offset++;
    }

    if (!foundFmt || !foundData) {
        m_lastError = "Missing fmt or data chunk";
        return false;
    }

    return true;
}

bool WwiseVorbisDecoder::ParseFmtChunk(const uint8_t* data, size_t size) {
    if (size < 18) {
        m_lastError = "fmt chunk too small";
        return false;
    }

    uint16_t formatTag = ReadU16LE(data);
    if (formatTag != 0xFFFF) {
        m_lastError = "Not Wwise Vorbis format (formatTag=" + std::to_string(formatTag) + ")";
        return false;
    }

    m_config.channels = ReadU16LE(data + 2);
    m_config.sampleRate = ReadU32LE(data + 4);

    uint16_t cbSize = ReadU16LE(data + 16);

    if (size < 18 + cbSize) {
        m_lastError = "fmt chunk truncated";
        return false;
    }

    // Parse extra data
    // Wwise fmt extra structure (varies by version):
    // +0x00: samples_per_block (2 bytes) - not always present
    // +0x02: ??? (2 bytes)
    // ...varies by version...

    const uint8_t* extra = data + 18;

    // The setup type indicator is typically at extra[2] in newer versions
    // But the actual structure varies significantly by Wwise version

    // Default to modern format (v62+)
    m_config.setupType = WwiseSetupType::AoTuV603Codebooks;
    m_config.headerType = WwiseHeaderType::Type2;
    m_config.packetType = WwisePacketType::Modified;

    if (cbSize >= 6) {
        // Try to detect version from extra data
        // extra[4] often contains setup type indicator in newer formats
        uint8_t setupIndicator = 0;
        if (cbSize >= 5) {
            setupIndicator = extra[4];
        }

        // Setup type detection based on common patterns
        if (setupIndicator == 0x00) {
            // Could be external codebooks or aoTuV603
            // Check for presence of setup_id pattern
            if (cbSize >= 10) {
                m_config.setupType = WwiseSetupType::AoTuV603Codebooks;
            }
        }
    }

    // Parse blocksize from extra data
    // In most Wwise versions, blocksizes are near the end of extra
    if (cbSize >= 4) {
        // Try common positions for blocksize
        // Position varies by version, but often at specific offsets

        // Check last 4 bytes for blocksize pattern
        uint8_t b0 = extra[cbSize - 4];
        uint8_t b1 = extra[cbSize - 3];

        // Validate as potential blocksizes (typically 6-13)
        if (b0 >= 6 && b0 <= 13 && b1 >= 6 && b1 <= 13 && b0 <= b1) {
            m_config.blocksize0Exp = b0;
            m_config.blocksize1Exp = b1;
        } else {
            // Try another common position
            if (cbSize >= 6) {
                b0 = extra[cbSize - 6];
                b1 = extra[cbSize - 5];
                if (b0 >= 6 && b0 <= 13 && b1 >= 6 && b1 <= 13 && b0 <= b1) {
                    m_config.blocksize0Exp = b0;
                    m_config.blocksize1Exp = b1;
                }
            }
        }

        // Default blocksizes if not found (most common in Wwise)
        if (m_config.blocksize0Exp < 6 || m_config.blocksize1Exp < 6) {
            m_config.blocksize0Exp = 8;  // 256 samples
            m_config.blocksize1Exp = 11; // 2048 samples
        }
    }

    // Total samples - position varies
    if (cbSize >= 10) {
        // Common position at offset 6 in extra
        m_config.totalSamples = ReadU32LE(extra + 6);
    }

    // Detect if blocksizes are equal (affects packet type)
    if (m_config.blocksize0Exp == m_config.blocksize1Exp) {
        m_config.packetType = WwisePacketType::Standard;
    }

    return true;
}

//=============================================================================
// Vorbis Header Builders
//=============================================================================
std::vector<uint8_t> WwiseVorbisDecoder::BuildIdentificationHeader() {
    std::vector<uint8_t> h(30);

    h[0] = 0x01;
    memcpy(&h[1], "vorbis", 6);
    // Version = 0 (bytes 7-10, already zero)
    h[11] = m_config.channels;
    h[12] = m_config.sampleRate & 0xFF;
    h[13] = (m_config.sampleRate >> 8) & 0xFF;
    h[14] = (m_config.sampleRate >> 16) & 0xFF;
    h[15] = (m_config.sampleRate >> 24) & 0xFF;
    // Bitrates = 0 (bytes 16-27, already zero)
    // blocksize: low nibble = blocksize_0, high nibble = blocksize_1
    h[28] = (m_config.blocksize0Exp) | (m_config.blocksize1Exp << 4);
    h[29] = 1; // Framing flag

    return h;
}

std::vector<uint8_t> WwiseVorbisDecoder::BuildCommentHeader() {
    std::vector<uint8_t> h;
    h.push_back(0x03);
    const char* id = "vorbis";
    h.insert(h.end(), id, id + 6);

    const char* vendor = "Converted";
    uint32_t vendorLen = strlen(vendor);
    h.push_back(vendorLen & 0xFF);
    h.push_back((vendorLen >> 8) & 0xFF);
    h.push_back((vendorLen >> 16) & 0xFF);
    h.push_back((vendorLen >> 24) & 0xFF);
    h.insert(h.end(), vendor, vendor + vendorLen);

    // No comments
    h.push_back(0); h.push_back(0); h.push_back(0); h.push_back(0);
    h.push_back(1); // Framing flag

    return h;
}

std::vector<uint8_t> WwiseVorbisDecoder::BuildSetupHeader() {
    uint16_t packetSize = 0;
    int32_t granule = 0;
    size_t headerSize = GetPacketHeader(m_config.dataOffset, packetSize, granule);

    if (headerSize == 0 || m_config.dataOffset + headerSize + packetSize > m_size) {
        m_lastError = "Invalid setup packet header";
        return {};
    }

    const uint8_t* setupData = m_data + m_config.dataOffset + headerSize;

    BitReader iw(setupData, packetSize);
    BitWriter ow;

    // Write Vorbis setup header prefix
    ow.Write(0x05, 8);
    const char* vorbisId = "vorbis";
    for (int i = 0; i < 6; i++) ow.Write(vorbisId[i], 8);

    // Codebook count
    uint32_t codebookCountMinus1 = iw.Read(8);
    ow.Write(codebookCountMinus1, 8);
    int codebookCount = codebookCountMinus1 + 1;

    // Process codebooks - KEY FIX: 10-bit is full codebook ID
    for (int i = 0; i < codebookCount; i++) {
        uint32_t codebookId = iw.Read(10);

        if (!GetRebuiltCodebook(codebookId, m_config.setupType, ow)) {
            m_lastError = "Failed to rebuild codebook ID " + std::to_string(codebookId);
            return {};
        }
    }

    // Time domain transforms (always 1 dummy)
    ow.Write(0, 6);
    ow.Write(0, 16);

    // Floors
    uint32_t floorCountMinus1 = iw.Read(6);
    ow.Write(floorCountMinus1, 6);

    for (uint32_t i = 0; i <= floorCountMinus1; i++) {
        ow.Write(1, 16); // Floor type 1

        uint32_t partitions = iw.Read(5);
        ow.Write(partitions, 5);

        uint32_t partitionClassList[32] = {0};
        int maxClass = -1;

        for (uint32_t j = 0; j < partitions; j++) {
            uint32_t partClass = iw.Read(4);
            ow.Write(partClass, 4);
            partitionClassList[j] = partClass;
            if ((int)partClass > maxClass) maxClass = (int)partClass;
        }

        uint32_t classDimensions[17] = {0};

        for (int j = 0; j <= maxClass; j++) {
            uint32_t dimMinus1 = iw.Read(3);
            ow.Write(dimMinus1, 3);
            classDimensions[j] = dimMinus1 + 1;

            uint32_t subclasses = iw.Read(2);
            ow.Write(subclasses, 2);

            if (subclasses != 0) {
                ow.Write(iw.Read(8), 8);
            }

            for (int k = 0; k < (1 << subclasses); k++) {
                ow.Write(iw.Read(8), 8);
            }
        }

        ow.Write(iw.Read(2), 2);
        uint32_t rangebits = iw.Read(4);
        ow.Write(rangebits, 4);

        for (uint32_t j = 0; j < partitions; j++) {
            for (uint32_t k = 0; k < classDimensions[partitionClassList[j]]; k++) {
                ow.Write(iw.Read(rangebits), rangebits);
            }
        }
    }

    // Residues
    uint32_t residueCountMinus1 = iw.Read(6);
    ow.Write(residueCountMinus1, 6);

    for (uint32_t i = 0; i <= residueCountMinus1; i++) {
        uint32_t residueType = iw.Read(2);
        ow.Write(residueType, 16);

        ow.Write(iw.Read(24), 24);
        ow.Write(iw.Read(24), 24);
        ow.Write(iw.Read(24), 24);

        uint32_t classificationsMinus1 = iw.Read(6);
        ow.Write(classificationsMinus1, 6);

        ow.Write(iw.Read(8), 8);

        uint32_t cascade[65] = {0};
        for (uint32_t j = 0; j <= classificationsMinus1; j++) {
            uint32_t lowBits = iw.Read(3);
            ow.Write(lowBits, 3);
            uint32_t bitflag = iw.Read(1);
            ow.Write(bitflag, 1);
            uint32_t highBits = 0;
            if (bitflag) {
                highBits = iw.Read(5);
                ow.Write(highBits, 5);
            }
            cascade[j] = highBits * 8 + lowBits;
        }

        for (uint32_t j = 0; j <= classificationsMinus1; j++) {
            for (int k = 0; k < 8; k++) {
                if (cascade[j] & (1 << k)) {
                    ow.Write(iw.Read(8), 8);
                }
            }
        }
    }

    // Mappings
    uint32_t mappingCountMinus1 = iw.Read(6);
    ow.Write(mappingCountMinus1, 6);

    for (uint32_t i = 0; i <= mappingCountMinus1; i++) {
        ow.Write(0, 16);

        uint32_t submapsFlag = iw.Read(1);
        ow.Write(submapsFlag, 1);

        uint32_t submaps = 1;
        if (submapsFlag) {
            uint32_t submapsMinus1 = iw.Read(4);
            ow.Write(submapsMinus1, 4);
            submaps = submapsMinus1 + 1;
        }

        uint32_t squarePolarFlag = iw.Read(1);
        ow.Write(squarePolarFlag, 1);

        if (squarePolarFlag) {
            uint32_t couplingStepsMinus1 = iw.Read(8);
            ow.Write(couplingStepsMinus1, 8);

            int couplingBits = ilog(m_config.channels - 1);
            if (couplingBits < 1) couplingBits = 1;

            for (uint32_t j = 0; j <= couplingStepsMinus1; j++) {
                ow.Write(iw.Read(couplingBits), couplingBits);
                ow.Write(iw.Read(couplingBits), couplingBits);
            }
        }

        uint32_t reserved = iw.Read(2);
        ow.Write(reserved, 2);

        if (submaps > 1) {
            for (int j = 0; j < m_config.channels; j++) {
                ow.Write(iw.Read(4), 4);
            }
        }

        for (uint32_t j = 0; j < submaps; j++) {
            ow.Write(iw.Read(8), 8);
            ow.Write(iw.Read(8), 8);
            ow.Write(iw.Read(8), 8);
        }
    }

    // Modes
    uint32_t modeCountMinus1 = iw.Read(6);
    ow.Write(modeCountMinus1, 6);

    m_modeBlockflag.resize(modeCountMinus1 + 1);
    m_modeBits = ilog(modeCountMinus1);
    if (m_modeBits < 1) m_modeBits = 1;

    for (uint32_t i = 0; i <= modeCountMinus1; i++) {
        uint32_t blockflag = iw.Read(1);
        ow.Write(blockflag, 1);
        m_modeBlockflag[i] = (blockflag != 0);

        ow.Write(0, 16);
        ow.Write(0, 16);
        ow.Write(iw.Read(8), 8);
    }

    // Framing bit
    ow.Write(1, 1);
    ow.FlushToByte();

    m_config.audioOffset = headerSize + packetSize;

    return ow.Data();
}

//=============================================================================
// Packet handling
//=============================================================================
size_t WwiseVorbisDecoder::GetPacketHeader(size_t offset, uint16_t& packetSize, int32_t& granule) {
    if (offset + 2 > m_size) return 0;

    switch (m_config.headerType) {
        case WwiseHeaderType::Type8:
            if (offset + 8 > m_size) return 0;
            packetSize = ReadU32LE(m_data + offset);
            granule = ReadU32LE(m_data + offset + 4);
            return 8;
        case WwiseHeaderType::Type6:
            if (offset + 6 > m_size) return 0;
            packetSize = ReadU16LE(m_data + offset);
            granule = ReadU32LE(m_data + offset + 2);
            return 6;
        case WwiseHeaderType::Type2:
        default:
            packetSize = ReadU16LE(m_data + offset);
            granule = 0;
            return 2;
    }
}

std::vector<uint8_t> WwiseVorbisDecoder::RebuildAudioPacket(size_t offset, size_t dataEnd, bool& hasNext, uint8_t& nextFirstByte) {
    uint16_t packetSize = 0;
    int32_t granule = 0;
    size_t headerSize = GetPacketHeader(offset, packetSize, granule);

    if (headerSize == 0 || offset + headerSize + packetSize > m_size) {
        return {};
    }

    const uint8_t* packetData = m_data + offset + headerSize;

    // Check if there's a next packet (for modified packet window flags)
    hasNext = false;
    nextFirstByte = 0;
    size_t nextOffset = offset + headerSize + packetSize;
    if (nextOffset + headerSize + 1 <= m_size && nextOffset < dataEnd) {
        uint16_t nextPacketSize = 0;
        int32_t nextGranule = 0;
        size_t nextHdrSize = GetPacketHeader(nextOffset, nextPacketSize, nextGranule);
        if (nextHdrSize > 0 && nextPacketSize > 0 && nextOffset + nextHdrSize < m_size) {
            hasNext = true;
            nextFirstByte = m_data[nextOffset + nextHdrSize];
        }
    }

    if (m_config.packetType == WwisePacketType::Standard) {
        return std::vector<uint8_t>(packetData, packetData + packetSize);
    }

    // Modified packets - rebuild first byte with window flags
    if (packetSize == 0) return {};

    BitReader iw(packetData, packetSize);
    BitWriter ow;

    // Audio packet type = 0
    ow.Write(0, 1);

    // Mode number (from Wwise packet, starts at bit 0)
    uint32_t modeNumber = iw.Read(m_modeBits);
    ow.Write(modeNumber, m_modeBits);

    // Remainder of first byte after mode bits
    uint32_t remainder = iw.Read(8 - m_modeBits);

    // Handle long blocks - add window flags
    if (modeNumber < m_modeBlockflag.size() && m_modeBlockflag[modeNumber]) {
        // Previous window type
        ow.Write(m_prevBlockflag ? 1 : 0, 1);

        // Next window type - peek at next packet
        uint32_t nextBlockflag = 0;
        if (hasNext) {
            // Read mode from next packet's first byte using bit reader
            BitReader nextReader(&nextFirstByte, 1);
            uint32_t nextModeNumber = nextReader.Read(m_modeBits);
            if (nextModeNumber < m_modeBlockflag.size()) {
                nextBlockflag = m_modeBlockflag[nextModeNumber] ? 1 : 0;
            }
        }
        ow.Write(nextBlockflag, 1);
    }

    m_prevBlockflag = (modeNumber < m_modeBlockflag.size()) ? m_modeBlockflag[modeNumber] : false;

    // Write remainder of first byte
    ow.Write(remainder, 8 - m_modeBits);

    // Copy rest of packet byte by byte (already byte-aligned after first byte)
    for (size_t i = 1; i < packetSize; i++) {
        ow.Write(packetData[i], 8);
    }

    ow.FlushToByte();
    return ow.Data();
}

//=============================================================================
// OGG Output
//=============================================================================
static void WriteOggPage(std::vector<uint8_t>& out, const std::vector<uint8_t>& data,
                         uint32_t serialNo, uint32_t& pageNo, int64_t granule, uint8_t flags) {
    size_t headerStart = out.size();

    out.push_back('O'); out.push_back('g'); out.push_back('g'); out.push_back('S');
    out.push_back(0);
    out.push_back(flags);

    for (int i = 0; i < 8; i++) out.push_back((granule >> (i * 8)) & 0xFF);
    for (int i = 0; i < 4; i++) out.push_back((serialNo >> (i * 8)) & 0xFF);
    for (int i = 0; i < 4; i++) out.push_back((pageNo >> (i * 8)) & 0xFF);
    pageNo++;

    size_t crcOffset = out.size();
    out.push_back(0); out.push_back(0); out.push_back(0); out.push_back(0);

    size_t numSegments = (data.size() == 0) ? 1 : ((data.size() + 254) / 255);
    out.push_back((uint8_t)numSegments);

    size_t remaining = data.size();
    for (size_t i = 0; i < numSegments; i++) {
        if (remaining >= 255) {
            out.push_back(255);
            remaining -= 255;
        } else {
            out.push_back((uint8_t)remaining);
            remaining = 0;
        }
    }

    out.insert(out.end(), data.begin(), data.end());

    out[crcOffset] = out[crcOffset+1] = out[crcOffset+2] = out[crcOffset+3] = 0;
    uint32_t crc = OggCRC32(out.data() + headerStart, out.size() - headerStart);
    out[crcOffset] = crc & 0xFF;
    out[crcOffset + 1] = (crc >> 8) & 0xFF;
    out[crcOffset + 2] = (crc >> 16) & 0xFF;
    out[crcOffset + 3] = (crc >> 24) & 0xFF;
}

bool WwiseVorbisDecoder::ConvertToOgg(std::vector<uint8_t>& outOgg) {
    outOgg.clear();

    uint32_t serialNo = 0x12345678;
    uint32_t pageNo = 0;

    auto idHeader = BuildIdentificationHeader();
    WriteOggPage(outOgg, idHeader, serialNo, pageNo, 0, 0x02);

    auto commentHeader = BuildCommentHeader();
    WriteOggPage(outOgg, commentHeader, serialNo, pageNo, 0, 0x00);

    auto setupHeader = BuildSetupHeader();
    if (setupHeader.empty()) {
        return false;
    }
    WriteOggPage(outOgg, setupHeader, serialNo, pageNo, 0, 0x00);

    size_t dataEnd = m_config.dataOffset + m_config.dataSize;
    size_t offset = m_config.dataOffset + m_config.audioOffset;
    int64_t granulePos = 0;
    int blocksize0 = 1 << m_config.blocksize0Exp;
    int blocksize1 = 1 << m_config.blocksize1Exp;

    m_prevBlockflag = false;
    int prevBlocksize = 0;

    while (offset < dataEnd) {
        uint16_t packetSize = 0;
        int32_t granule = 0;
        size_t headerSize = GetPacketHeader(offset, packetSize, granule);

        if (headerSize == 0 || packetSize == 0 || offset + headerSize + packetSize > m_size) {
            break;
        }

        bool hasNext = false;
        uint8_t nextFirstByte = 0;
        auto audioPacket = RebuildAudioPacket(offset, dataEnd, hasNext, nextFirstByte);
        if (audioPacket.empty()) {
            offset += headerSize + packetSize;
            continue;
        }

        // Calculate granule position from packet
        // Read mode from rebuilt packet to determine blocksize
        if (!audioPacket.empty()) {
            // In rebuilt packet: bit 0 = packet type, bits 1+ = mode
            BitReader packetReader(audioPacket.data(), audioPacket.size());
            packetReader.Read(1); // skip packet type
            uint32_t modeNum = packetReader.Read(m_modeBits);

            bool thisBlockflag = (modeNum < m_modeBlockflag.size()) ? m_modeBlockflag[modeNum] : false;
            int thisBlocksize = thisBlockflag ? blocksize1 : blocksize0;

            if (prevBlocksize > 0) {
                granulePos += (prevBlocksize + thisBlocksize) / 4;
            }
            prevBlocksize = thisBlocksize;
        }

        WriteOggPage(outOgg, audioPacket, serialNo, pageNo, granulePos, 0x00);

        offset += headerSize + packetSize;
    }

    // Mark last page as EOS by re-writing with EOS flag
    // (simplified - in production would properly track and write last page)

    return !outOgg.empty();
}

} // namespace Audio