#include "WwiseVorbisDecoder.h"
#include "WwiseCodebookData.h"
#include <cstring>
#include <fstream>
#include <algorithm>

namespace Audio {

class CodebookLibrary {
public:
    bool LoadFromMemory(const uint8_t* data, size_t size);
    bool LoadFromFile(const std::string& path);
    bool IsLoaded() const { return m_loaded; }
    const char* GetCodebook(int id) const;
    long GetCodebookSize(int id) const;

private:
    bool m_loaded = false;
    std::vector<char> m_data;
    std::vector<long> m_offsets;
    long m_count = 0;
};

static CodebookLibrary g_codebooks;

static inline uint16_t ReadU16LE(const uint8_t* p) { return p[0] | (p[1] << 8); }
static inline uint32_t ReadU32LE(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static inline uint16_t ReadU16BE(const uint8_t* p) { return (p[0] << 8) | p[1]; }
static inline uint32_t ReadU32BE(const uint8_t* p) { return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

static int ilog(unsigned int v) {
    int ret = 0;
    while (v) { ret++; v >>= 1; }
    return ret;
}

static unsigned int book_maptype1_quantvals(unsigned int entries, unsigned int dimensions) {
    int bits = ilog(entries);
    int vals = entries >> ((bits - 1) * (dimensions - 1) / dimensions);
    while (1) {
        unsigned long acc = 1, acc1 = 1;
        for (unsigned int i = 0; i < dimensions; i++) {
            acc *= vals;
            acc1 *= vals + 1;
        }
        if (acc <= entries && acc1 > entries) return vals;
        if (acc > entries) vals--;
        else vals++;
    }
}

bool WwiseInitEmbedded() {
    return g_codebooks.LoadFromMemory(g_wwiseCodebookData, g_wwiseCodebookSize);
}

bool WwiseInit(const std::string& codebooksPath, const std::string& logPath) {
    return g_codebooks.LoadFromFile(codebooksPath);
}

void WwiseShutdown() {}

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size) : m_data(data), m_size(size), m_bitPos(0) {}

    unsigned int Read(int bits) {
        unsigned int result = 0;
        for (int i = 0; i < bits; i++) {
            size_t bytePos = m_bitPos / 8;
            int bitInByte = m_bitPos % 8;
            if (bytePos < m_size) {
                if (m_data[bytePos] & (1 << bitInByte))
                    result |= (1u << i);
            }
            m_bitPos++;
        }
        return result;
    }

    size_t GetTotalBitsRead() const { return m_bitPos; }

private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_bitPos;
};

static const uint32_t crc_lookup[256] = {
    0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
    0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
    0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
    0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
    0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
    0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
    0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
    0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
    0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
    0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
    0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
    0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
    0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
    0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
    0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
    0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
    0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
    0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
    0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
    0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
    0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
    0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
    0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
    0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
    0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
    0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
    0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
    0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
    0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
    0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
    0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
    0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4
};

static uint32_t OggCRC32(const uint8_t* data, size_t size) {
    uint32_t crc = 0;
    for (size_t i = 0; i < size; i++)
        crc = (crc << 8) ^ crc_lookup[((crc >> 24) & 0xff) ^ data[i]];
    return crc;
}

class OggWriter {
public:
    OggWriter(std::vector<uint8_t>& output) : m_output(output), m_seqNo(0) {}

    void SetGranule(uint64_t g) { m_granule = g; }

    void WriteBit(bool bit) {
        if (bit) m_bitBuffer |= (1 << m_bitsStored);
        m_bitsStored++;
        if (m_bitsStored == 8) FlushBits();
    }

    void WriteBits(unsigned int value, int bits) {
        for (int i = 0; i < bits; i++)
            WriteBit((value & (1u << i)) != 0);
    }

    void FlushBits() {
        if (m_bitsStored != 0) {
            m_payload.push_back(m_bitBuffer);
            m_bitBuffer = 0;
            m_bitsStored = 0;
        }
    }

    void FlushPage(bool continued = false, bool first = false, bool last = false) {
        FlushBits();
        if (m_payload.empty()) return;

        size_t payloadSize = m_payload.size();
        size_t segments = (payloadSize + 254) / 255;
        if (segments > 255) segments = 255;

        std::vector<uint8_t> page;
        page.reserve(27 + segments + payloadSize);

        page.push_back('O'); page.push_back('g'); page.push_back('g'); page.push_back('S');
        page.push_back(0);

        uint8_t flags = 0;
        if (continued) flags |= 0x01;
        if (first) flags |= 0x02;
        if (last) flags |= 0x04;
        page.push_back(flags);

        for (int i = 0; i < 8; i++) page.push_back((m_granule >> (i * 8)) & 0xFF);

        page.push_back(1); page.push_back(0); page.push_back(0); page.push_back(0);

        for (int i = 0; i < 4; i++) page.push_back((m_seqNo >> (i * 8)) & 0xFF);
        m_seqNo++;

        size_t crcOffset = page.size();
        page.push_back(0); page.push_back(0); page.push_back(0); page.push_back(0);

        page.push_back((uint8_t)segments);

        size_t remaining = payloadSize;
        for (size_t i = 0; i < segments; i++) {
            if (remaining >= 255) {
                page.push_back(255);
                remaining -= 255;
            } else {
                page.push_back((uint8_t)remaining);
                remaining = 0;
            }
        }

        page.insert(page.end(), m_payload.begin(), m_payload.end());

        uint32_t crc = OggCRC32(page.data(), page.size());
        page[crcOffset + 0] = crc & 0xFF;
        page[crcOffset + 1] = (crc >> 8) & 0xFF;
        page[crcOffset + 2] = (crc >> 16) & 0xFF;
        page[crcOffset + 3] = (crc >> 24) & 0xFF;

        m_output.insert(m_output.end(), page.begin(), page.end());
        m_payload.clear();
    }

private:
    std::vector<uint8_t>& m_output;
    std::vector<uint8_t> m_payload;
    uint64_t m_granule = 0;
    uint32_t m_seqNo = 0;
    uint8_t m_bitBuffer = 0;
    int m_bitsStored = 0;
};

bool CodebookLibrary::LoadFromMemory(const uint8_t* data, size_t size) {
    if (size < 8) return false;

    long offsetOffset = ReadU32LE(data + size - 4);
    m_count = (size - offsetOffset) / 4;

    m_data.resize(offsetOffset);
    memcpy(m_data.data(), data, offsetOffset);

    m_offsets.resize(m_count);
    const uint8_t* offsetPtr = data + offsetOffset;
    for (long i = 0; i < m_count; i++) {
        m_offsets[i] = ReadU32LE(offsetPtr + i * 4);
    }

    m_loaded = true;
    return true;
}

bool CodebookLibrary::LoadFromFile(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is) return false;

    is.seekg(0, std::ios::end);
    long fileSize = (long)is.tellg();

    is.seekg(fileSize - 4);
    uint8_t buf[4];
    is.read((char*)buf, 4);
    long offsetOffset = ReadU32LE(buf);

    m_count = (fileSize - offsetOffset) / 4;

    m_data.resize(offsetOffset);
    is.seekg(0);
    is.read(m_data.data(), offsetOffset);

    m_offsets.resize(m_count);
    for (long i = 0; i < m_count; i++) {
        is.read((char*)buf, 4);
        m_offsets[i] = ReadU32LE(buf);
    }

    m_loaded = true;
    return true;
}

const char* CodebookLibrary::GetCodebook(int id) const {
    if (!m_loaded || id < 0 || id >= m_count - 1) return nullptr;
    return &m_data[m_offsets[id]];
}

long CodebookLibrary::GetCodebookSize(int id) const {
    if (!m_loaded || id < 0 || id >= m_count - 1) return -1;
    return m_offsets[id + 1] - m_offsets[id];
}

static bool RebuildCodebook(BitReader& in, OggWriter& out) {
    out.WriteBits(0x564342, 24);

    unsigned int dimensions = in.Read(4);
    out.WriteBits(dimensions, 16);

    unsigned int entries = in.Read(14);
    out.WriteBits(entries, 24);

    unsigned int ordered = in.Read(1);
    out.WriteBits(ordered, 1);

    if (ordered) {
        unsigned int initialLength = in.Read(5);
        out.WriteBits(initialLength, 5);

        unsigned int currentEntry = 0;
        while (currentEntry < entries) {
            int numberBits = ilog(entries - currentEntry);
            unsigned int number = in.Read(numberBits);
            out.WriteBits(number, numberBits);
            currentEntry += number;
        }
        if (currentEntry > entries) return false;
    } else {
        unsigned int codewordLengthLength = in.Read(3);
        unsigned int sparse = in.Read(1);

        out.WriteBits(sparse, 1);

        if (codewordLengthLength == 0 || codewordLengthLength > 5) return false;

        for (unsigned int i = 0; i < entries; i++) {
            bool present = true;
            if (sparse) {
                unsigned int p = in.Read(1);
                out.WriteBits(p, 1);
                present = (p != 0);
            }
            if (present) {
                unsigned int len = in.Read(codewordLengthLength);
                out.WriteBits(len, 5);
            }
        }
    }

    unsigned int lookupType = in.Read(1);
    out.WriteBits(lookupType, 4);

    if (lookupType == 1) {
        out.WriteBits(in.Read(32), 32);
        out.WriteBits(in.Read(32), 32);
        unsigned int valueLength = in.Read(4);
        out.WriteBits(valueLength, 4);
        out.WriteBits(in.Read(1), 1);

        unsigned int quantvals = book_maptype1_quantvals(entries, dimensions);
        for (unsigned int i = 0; i < quantvals; i++)
            out.WriteBits(in.Read(valueLength + 1), valueLength + 1);
    } else if (lookupType != 0) {
        return false;
    }

    return true;
}

static bool RebuildCodebookById(int id, OggWriter& out) {
    const char* cb = g_codebooks.GetCodebook(id);
    long size = g_codebooks.GetCodebookSize(id);
    if (!cb || size <= 0) return false;
    BitReader in((const uint8_t*)cb, size);
    return RebuildCodebook(in, out);
}

bool WwiseVorbisDecoder::ParseWEM(const uint8_t* data, size_t size) {
    m_data = data;
    m_size = size;

    if (size < 12) {
        m_lastError = "File too small";
        return false;
    }

    if (memcmp(data, "RIFF", 4) == 0) {
        m_littleEndian = true;
    } else if (memcmp(data, "RIFX", 4) == 0) {
        m_littleEndian = false;
    } else {
        m_lastError = "Missing RIFF header";
        return false;
    }

    auto read16 = m_littleEndian ? ReadU16LE : ReadU16BE;
    auto read32 = m_littleEndian ? ReadU32LE : ReadU32BE;

    uint32_t riffSize = read32(data + 4) + 8;
    if (riffSize > size) {
        m_lastError = "RIFF truncated";
        return false;
    }

    if (memcmp(data + 8, "WAVE", 4) != 0) {
        m_lastError = "Missing WAVE";
        return false;
    }

    size_t offset = 12;
    while (offset + 8 <= riffSize) {
        const uint8_t* chunk = data + offset;
        uint32_t chunkSize = read32(chunk + 4);

        if (memcmp(chunk, "fmt ", 4) == 0) {
            m_fmtOffset = offset + 8;
            m_fmtSize = chunkSize;
        } else if (memcmp(chunk, "vorb", 4) == 0) {
            m_vorbOffset = offset + 8;
            m_vorbSize = chunkSize;
        } else if (memcmp(chunk, "data", 4) == 0) {
            m_dataOffset = offset + 8;
            m_dataSize = chunkSize;
        }
        offset += 8 + chunkSize;
    }

    if (m_fmtOffset < 0 || m_dataOffset < 0) {
        m_lastError = "Missing fmt or data chunk";
        return false;
    }

    const uint8_t* fmt = data + m_fmtOffset;
    if (read16(fmt) != 0xFFFF) {
        m_lastError = "Bad codec ID";
        return false;
    }
    m_channels = read16(fmt + 2);
    m_sampleRate = read32(fmt + 4);
    m_avgBytesPerSec = read32(fmt + 8);

    if (m_vorbOffset < 0 && m_fmtSize == 0x42) {
        m_vorbOffset = m_fmtOffset + 0x18;
        m_vorbSize = 0x2A;
    }

    if (m_vorbOffset < 0) {
        m_lastError = "Missing vorb chunk";
        return false;
    }

    const uint8_t* vorb = data + m_vorbOffset;
    m_sampleCount = read32(vorb);

    switch (m_vorbSize) {
        case 0x2A:
        case -1: {
            m_noGranule = true;
            uint32_t modSignal = read32(vorb + 0x4);
            if (modSignal != 0x4A && modSignal != 0x4B && modSignal != 0x69 && modSignal != 0x70) {
                m_modPackets = true;
            }
            m_setupPacketOffset = read32(vorb + 0x10);
            m_firstAudioPacketOffset = read32(vorb + 0x14);
            m_blocksize0Pow = vorb[0x28];
            m_blocksize1Pow = vorb[0x29];
            break;
        }
        case 0x32:
        case 0x34:
            m_setupPacketOffset = read32(vorb + 0x18);
            m_firstAudioPacketOffset = read32(vorb + 0x1C);
            m_blocksize0Pow = vorb[0x30];
            m_blocksize1Pow = vorb[0x31];
            break;
        default:
            m_lastError = "Unknown vorb size: " + std::to_string(m_vorbSize);
            return false;
    }

    return true;
}

bool WwiseVorbisDecoder::ConvertToOgg(std::vector<uint8_t>& outOgg) {
    if (!g_codebooks.IsLoaded()) {
        m_lastError = "Codebooks not loaded";
        return false;
    }

    outOgg.clear();
    OggWriter ogg(outOgg);

    auto read16 = m_littleEndian ? ReadU16LE : ReadU16BE;
    auto read32 = m_littleEndian ? ReadU32LE : ReadU32BE;

    ogg.WriteBits(1, 8);
    const char* vorbis = "vorbis";
    for (int i = 0; i < 6; i++) ogg.WriteBits(vorbis[i], 8);
    ogg.WriteBits(0, 32);
    ogg.WriteBits(m_channels, 8);
    ogg.WriteBits(m_sampleRate, 32);
    ogg.WriteBits(0, 32);
    ogg.WriteBits(m_avgBytesPerSec * 8, 32);
    ogg.WriteBits(0, 32);
    ogg.WriteBits(m_blocksize0Pow, 4);
    ogg.WriteBits(m_blocksize1Pow, 4);
    ogg.WriteBits(1, 1);
    ogg.FlushPage(false, true, false);

    ogg.WriteBits(3, 8);
    for (int i = 0; i < 6; i++) ogg.WriteBits(vorbis[i], 8);
    const char* vendor = "ww2ogg";
    ogg.WriteBits(strlen(vendor), 32);
    for (size_t i = 0; i < strlen(vendor); i++) ogg.WriteBits(vendor[i], 8);
    ogg.WriteBits(0, 32);
    ogg.WriteBits(1, 1);
    ogg.FlushPage();

    ogg.WriteBits(5, 8);
    for (int i = 0; i < 6; i++) ogg.WriteBits(vorbis[i], 8);

    size_t setupOffset = m_dataOffset + m_setupPacketOffset;
    uint16_t setupSize = read16(m_data + setupOffset);
    size_t setupDataOffset = setupOffset + (m_noGranule ? 2 : 6);

    BitReader setupIn(m_data + setupDataOffset, setupSize);

    unsigned int codebookCount = setupIn.Read(8) + 1;
    ogg.WriteBits(codebookCount - 1, 8);

    for (unsigned int i = 0; i < codebookCount; i++) {
        unsigned int codebookId = setupIn.Read(10);
        if (!RebuildCodebookById(codebookId, ogg)) {
            m_lastError = "Failed to rebuild codebook " + std::to_string(codebookId);
            return false;
        }
    }

    ogg.WriteBits(0, 6);
    ogg.WriteBits(0, 16);

    unsigned int floorCount = setupIn.Read(6) + 1;
    ogg.WriteBits(floorCount - 1, 6);

    for (unsigned int i = 0; i < floorCount; i++) {
        ogg.WriteBits(1, 16);

        unsigned int partitions = setupIn.Read(5);
        ogg.WriteBits(partitions, 5);

        std::vector<unsigned int> partitionClassList(partitions);
        unsigned int maxClass = 0;

        for (unsigned int j = 0; j < partitions; j++) {
            unsigned int partClass = setupIn.Read(4);
            ogg.WriteBits(partClass, 4);
            partitionClassList[j] = partClass;
            if (partClass > maxClass) maxClass = partClass;
        }

        std::vector<unsigned int> classDimensions(maxClass + 1);

        for (unsigned int j = 0; j <= maxClass; j++) {
            unsigned int dimMinus1 = setupIn.Read(3);
            ogg.WriteBits(dimMinus1, 3);
            classDimensions[j] = dimMinus1 + 1;

            unsigned int subclasses = setupIn.Read(2);
            ogg.WriteBits(subclasses, 2);

            if (subclasses) {
                ogg.WriteBits(setupIn.Read(8), 8);
            }

            for (unsigned int k = 0; k < (1u << subclasses); k++) {
                ogg.WriteBits(setupIn.Read(8), 8);
            }
        }

        ogg.WriteBits(setupIn.Read(2), 2);
        unsigned int rangebits = setupIn.Read(4);
        ogg.WriteBits(rangebits, 4);

        for (unsigned int j = 0; j < partitions; j++) {
            for (unsigned int k = 0; k < classDimensions[partitionClassList[j]]; k++) {
                ogg.WriteBits(setupIn.Read(rangebits), rangebits);
            }
        }
    }

    unsigned int residueCount = setupIn.Read(6) + 1;
    ogg.WriteBits(residueCount - 1, 6);

    for (unsigned int i = 0; i < residueCount; i++) {
        unsigned int residueType = setupIn.Read(2);
        ogg.WriteBits(residueType, 16);

        ogg.WriteBits(setupIn.Read(24), 24);
        ogg.WriteBits(setupIn.Read(24), 24);
        ogg.WriteBits(setupIn.Read(24), 24);
        unsigned int classifications = setupIn.Read(6) + 1;
        ogg.WriteBits(classifications - 1, 6);
        ogg.WriteBits(setupIn.Read(8), 8);

        std::vector<unsigned int> cascade(classifications);
        for (unsigned int j = 0; j < classifications; j++) {
            unsigned int lowBits = setupIn.Read(3);
            ogg.WriteBits(lowBits, 3);
            unsigned int bitflag = setupIn.Read(1);
            ogg.WriteBits(bitflag, 1);
            unsigned int highBits = 0;
            if (bitflag) {
                highBits = setupIn.Read(5);
                ogg.WriteBits(highBits, 5);
            }
            cascade[j] = highBits * 8 + lowBits;
        }

        for (unsigned int j = 0; j < classifications; j++) {
            for (int k = 0; k < 8; k++) {
                if (cascade[j] & (1 << k)) {
                    ogg.WriteBits(setupIn.Read(8), 8);
                }
            }
        }
    }

    unsigned int mappingCount = setupIn.Read(6) + 1;
    ogg.WriteBits(mappingCount - 1, 6);

    for (unsigned int i = 0; i < mappingCount; i++) {
        ogg.WriteBits(0, 16);

        unsigned int submapsFlag = setupIn.Read(1);
        ogg.WriteBits(submapsFlag, 1);

        unsigned int submaps = 1;
        if (submapsFlag) {
            submaps = setupIn.Read(4) + 1;
            ogg.WriteBits(submaps - 1, 4);
        }

        unsigned int squarePolarFlag = setupIn.Read(1);
        ogg.WriteBits(squarePolarFlag, 1);

        if (squarePolarFlag) {
            unsigned int couplingSteps = setupIn.Read(8) + 1;
            ogg.WriteBits(couplingSteps - 1, 8);

            int channelBits = ilog(m_channels - 1);
            for (unsigned int j = 0; j < couplingSteps; j++) {
                ogg.WriteBits(setupIn.Read(channelBits), channelBits);
                ogg.WriteBits(setupIn.Read(channelBits), channelBits);
            }
        }

        unsigned int reserved = setupIn.Read(2);
        ogg.WriteBits(reserved, 2);

        if (submaps > 1) {
            for (unsigned int j = 0; j < m_channels; j++) {
                ogg.WriteBits(setupIn.Read(4), 4);
            }
        }

        for (unsigned int j = 0; j < submaps; j++) {
            ogg.WriteBits(setupIn.Read(8), 8);
            ogg.WriteBits(setupIn.Read(8), 8);
            ogg.WriteBits(setupIn.Read(8), 8);
        }
    }

    unsigned int modeCount = setupIn.Read(6) + 1;
    ogg.WriteBits(modeCount - 1, 6);

    m_modeBlockflag.resize(modeCount);
    m_modeBits = ilog(modeCount - 1);

    for (unsigned int i = 0; i < modeCount; i++) {
        unsigned int blockflag = setupIn.Read(1);
        ogg.WriteBits(blockflag, 1);
        m_modeBlockflag[i] = (blockflag != 0);

        ogg.WriteBits(0, 16);
        ogg.WriteBits(0, 16);
        ogg.WriteBits(setupIn.Read(8), 8);
    }

    ogg.WriteBits(1, 1);
    ogg.FlushPage();

    size_t audioOffset = m_dataOffset + m_firstAudioPacketOffset;
    size_t dataEnd = m_dataOffset + m_dataSize;
    bool prevBlockflag = false;
    uint64_t granulePos = 0;

    while (audioOffset < dataEnd) {
        uint16_t packetSize = read16(m_data + audioOffset);
        uint32_t packetGranule = 0;
        size_t headerSize = m_noGranule ? 2 : 6;

        if (!m_noGranule) {
            packetGranule = read32(m_data + audioOffset + 2);
        }

        if (audioOffset + headerSize + packetSize > dataEnd) break;

        const uint8_t* packetData = m_data + audioOffset + headerSize;

        if (m_noGranule) {
            granulePos += (1 << m_blocksize0Pow) / 4;
            ogg.SetGranule(granulePos);
        } else {
            ogg.SetGranule(packetGranule == 0xFFFFFFFF ? 0 : packetGranule);
        }

        if (m_modPackets && m_modeBits > 0) {
            ogg.WriteBits(0, 1);

            BitReader packetIn(packetData, packetSize);
            unsigned int modeNumber = packetIn.Read(m_modeBits);
            ogg.WriteBits(modeNumber, m_modeBits);

            unsigned int remainder = packetIn.Read(8 - m_modeBits);

            if (modeNumber < m_modeBlockflag.size() && m_modeBlockflag[modeNumber]) {
                bool nextBlockflag = false;

                size_t nextOffset = audioOffset + headerSize + packetSize;
                if (nextOffset + headerSize <= dataEnd) {
                    uint16_t nextSize = read16(m_data + nextOffset);
                    if (nextSize > 0) {
                        const uint8_t* nextData = m_data + nextOffset + headerSize;
                        BitReader nextIn(nextData, nextSize);
                        unsigned int nextMode = nextIn.Read(m_modeBits);
                        if (nextMode < m_modeBlockflag.size()) {
                            nextBlockflag = m_modeBlockflag[nextMode];
                        }
                    }
                }

                ogg.WriteBits(prevBlockflag ? 1 : 0, 1);
                ogg.WriteBits(nextBlockflag ? 1 : 0, 1);
            }

            if (modeNumber < m_modeBlockflag.size()) {
                prevBlockflag = m_modeBlockflag[modeNumber];
            }

            ogg.WriteBits(remainder, 8 - m_modeBits);

            for (size_t i = 1; i < packetSize; i++) {
                ogg.WriteBits(packetData[i], 8);
            }
        } else {
            for (size_t i = 0; i < packetSize; i++) {
                ogg.WriteBits(packetData[i], 8);
            }
        }

        audioOffset += headerSize + packetSize;
        ogg.FlushPage(false, false, audioOffset >= dataEnd);
    }

    return true;
}

}