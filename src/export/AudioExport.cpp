#include "AudioExport.h"
#include "wwise/WwiseVorbisDecoder.h"
#include <fstream>
#include <cstring>
#include <vector>

struct stb_vorbis;
struct stb_vorbis_info {
    unsigned int sample_rate;
    int channels;
    unsigned int setup_memory_required;
    unsigned int setup_temp_memory_required;
    unsigned int temp_memory_required;
    int max_frame_size;
};

extern "C" {
    stb_vorbis* stb_vorbis_open_memory(const unsigned char* data, int len, int* error, const void* alloc);
    stb_vorbis_info stb_vorbis_get_info(stb_vorbis* f);
    int stb_vorbis_get_samples_short_interleaved(stb_vorbis* f, int channels, short* buffer, int num_shorts);
    void stb_vorbis_close(stb_vorbis* f);
}

namespace Audio {

std::string AudioExport::s_lastError;

static inline void WriteU16LE(uint8_t* p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}

static inline void WriteU32LE(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
}

static inline uint16_t ReadU16LE(const uint8_t* p) { return p[0] | (p[1] << 8); }
static inline uint32_t ReadU32LE(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }
static inline uint16_t ReadU16BE(const uint8_t* p) { return (p[0] << 8) | p[1]; }
static inline uint32_t ReadU32BE(const uint8_t* p) { return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

static bool DecodeWEM(const uint8_t* wemData, size_t wemSize, std::vector<int16_t>& pcmOut, uint16_t& channelsOut, uint32_t& sampleRateOut, std::string& errorOut) {
    pcmOut.clear();

    if (wemSize < 12) {
        errorOut = "File too small";
        return false;
    }

    bool isRIFF = memcmp(wemData, "RIFF", 4) == 0;
    bool isRIFX = memcmp(wemData, "RIFX", 4) == 0;

    if (!isRIFF && !isRIFX) {
        errorOut = "Not RIFF/RIFX";
        return false;
    }
    if (memcmp(wemData + 8, "WAVE", 4) != 0) {
        errorOut = "Not WAVE";
        return false;
    }

    bool bigEndian = isRIFX;
    auto RU16 = bigEndian ? ReadU16BE : ReadU16LE;
    auto RU32 = bigEndian ? ReadU32BE : ReadU32LE;

    size_t offset = 12;
    uint16_t formatTag = 0, channels = 0, blockAlign = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    const uint8_t* audioData = nullptr;
    size_t audioSize = 0;

    while (offset + 8 <= wemSize) {
        char chunkId[5] = {0};
        memcpy(chunkId, wemData + offset, 4);
        uint32_t chunkSize = RU32(wemData + offset + 4);

        if (offset + 8 + chunkSize > wemSize) break;

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            const uint8_t* fmt = wemData + offset + 8;
            formatTag = RU16(fmt);
            channels = RU16(fmt + 2);
            sampleRate = RU32(fmt + 4);
            blockAlign = RU16(fmt + 12);
            bitsPerSample = RU16(fmt + 14);
        } else if (memcmp(chunkId, "data", 4) == 0) {
            audioData = wemData + offset + 8;
            audioSize = chunkSize;
        }

        offset += 8 + chunkSize;
        if (offset % 2) offset++;
    }

    if (!audioData || audioSize == 0) {
        errorOut = "No audio data";
        return false;
    }

    channelsOut = channels;
    sampleRateOut = sampleRate;

    if (formatTag == 0x0001) {
        if (bitsPerSample == 16) {
            size_t numSamples = audioSize / 2;
            pcmOut.resize(numSamples);
            memcpy(pcmOut.data(), audioData, audioSize);
            return true;
        } else {
            errorOut = "Unsupported PCM bits: " + std::to_string(bitsPerSample);
            return false;
        }
    } else if (formatTag == 0x0002) {
        static const int AdaptTable[] = { 230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230 };
        static const int Coef1[] = { 256, 512, 0, 192, 240, 460, 392 };
        static const int Coef2[] = { 0, -256, 0, 64, 0, -208, -232 };

        if (blockAlign == 0 || channels == 0) {
            errorOut = "Invalid ADPCM params";
            return false;
        }

        size_t numBlocks = audioSize / blockAlign;
        size_t samplesPerBlock = (blockAlign - 7 * channels) * 2 / channels + 2;
        pcmOut.reserve(numBlocks * samplesPerBlock * channels);

        for (size_t blk = 0; blk < numBlocks; blk++) {
            const uint8_t* bd = audioData + blk * blockAlign;
            size_t pos = 0;

            std::vector<int> c1(channels), c2(channels), delta(channels), s1(channels), s2(channels);

            for (int ch = 0; ch < channels; ch++) {
                int pred = bd[pos++];
                if (pred > 6) pred = 6;
                c1[ch] = Coef1[pred];
                c2[ch] = Coef2[pred];
            }
            for (int ch = 0; ch < channels; ch++) { delta[ch] = (int16_t)(bd[pos] | (bd[pos+1] << 8)); pos += 2; }
            for (int ch = 0; ch < channels; ch++) { s1[ch] = (int16_t)(bd[pos] | (bd[pos+1] << 8)); pos += 2; }
            for (int ch = 0; ch < channels; ch++) { s2[ch] = (int16_t)(bd[pos] | (bd[pos+1] << 8)); pos += 2; }

            if (channels == 1) {
                pcmOut.push_back((int16_t)s2[0]);
                pcmOut.push_back((int16_t)s1[0]);
            } else {
                pcmOut.push_back((int16_t)s2[0]);
                pcmOut.push_back((int16_t)s2[1]);
                pcmOut.push_back((int16_t)s1[0]);
                pcmOut.push_back((int16_t)s1[1]);
            }

            while (pos < blockAlign) {
                for (int ch = 0; ch < channels && pos < blockAlign; ch++) {
                    uint8_t byte = bd[pos++];
                    for (int n = 0; n < 2; n++) {
                        int nib = (n == 0) ? ((byte >> 4) & 0xF) : (byte & 0xF);
                        int snib = (nib >= 8) ? (nib - 16) : nib;
                        int pred = ((s1[ch] * c1[ch]) + (s2[ch] * c2[ch])) / 256 + snib * delta[ch];
                        if (pred > 32767) pred = 32767;
                        if (pred < -32768) pred = -32768;
                        s2[ch] = s1[ch];
                        s1[ch] = pred;
                        delta[ch] = (AdaptTable[nib] * delta[ch]) / 256;
                        if (delta[ch] < 16) delta[ch] = 16;
                        pcmOut.push_back((int16_t)pred);
                    }
                }
            }
        }
        return true;
    } else if (formatTag == 0xFFFF || formatTag == 0xFFFE) {
        WwiseVorbisDecoder decoder;
        if (!decoder.ParseWEM(wemData, wemSize)) {
            errorOut = "WEM parse failed: " + decoder.GetLastError();
            return false;
        }

        channelsOut = decoder.GetChannels();
        sampleRateOut = decoder.GetSampleRate();

        std::vector<uint8_t> oggData;
        if (!decoder.ConvertToOgg(oggData)) {
            errorOut = "OGG conversion failed: " + decoder.GetLastError();
            return false;
        }

        int stbError = 0;
        stb_vorbis* vorbis = stb_vorbis_open_memory(oggData.data(), (int)oggData.size(), &stbError, nullptr);

        if (!vorbis) {
            errorOut = "Vorbis open failed (code " + std::to_string(stbError) + ")";
            return false;
        }

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        channelsOut = info.channels;
        sampleRateOut = info.sample_rate;

        pcmOut.reserve(1024 * 1024);

        short tempBuf[4096];
        int samplesRead;
        while ((samplesRead = stb_vorbis_get_samples_short_interleaved(vorbis, info.channels, tempBuf, 4096)) > 0) {
            pcmOut.insert(pcmOut.end(), tempBuf, tempBuf + samplesRead * info.channels);
        }
        stb_vorbis_close(vorbis);

        if (pcmOut.empty()) {
            errorOut = "No samples decoded";
            return false;
        }

        return true;
    } else {
        errorOut = "Unsupported format: 0x" + std::to_string(formatTag);
        return false;
    }
}

bool AudioExport::ExportWEMToWAV(const uint8_t* wemData, size_t wemSize, const std::string& filepath) {
    std::vector<int16_t> pcm;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;

    if (!DecodeWEM(wemData, wemSize, pcm, channels, sampleRate, s_lastError)) {
        return false;
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        s_lastError = "Failed to create file: " + filepath;
        return false;
    }

    uint16_t bitsPerSample = 16;
    uint16_t blockAlign = channels * (bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;
    uint32_t dataSize = (uint32_t)(pcm.size() * 2);
    uint32_t fileSize = 36 + dataSize;

    uint8_t header[44];

    memcpy(header, "RIFF", 4);
    WriteU32LE(header + 4, fileSize);
    memcpy(header + 8, "WAVE", 4);

    memcpy(header + 12, "fmt ", 4);
    WriteU32LE(header + 16, 16);
    WriteU16LE(header + 20, 1);
    WriteU16LE(header + 22, channels);
    WriteU32LE(header + 24, sampleRate);
    WriteU32LE(header + 28, byteRate);
    WriteU16LE(header + 32, blockAlign);
    WriteU16LE(header + 34, bitsPerSample);

    memcpy(header + 36, "data", 4);
    WriteU32LE(header + 40, dataSize);

    file.write((const char*)header, 44);
    file.write((const char*)pcm.data(), dataSize);

    s_lastError.clear();
    return true;
}

}