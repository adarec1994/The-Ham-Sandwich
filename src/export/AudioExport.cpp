#include "AudioExport.h"
#include "../UI/FileOps.h"
#include "Wwise/WwiseVorbisDecoder.h"
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <filesystem>

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
    } else if (formatTag == 0x0002 && bitsPerSample == 4) {
        // Wwise ADPCM (format 0x0002 with 4-bit) - uses IMA ADPCM algorithm
        static const int IMA_IndexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
        static const int IMA_StepTable[89] = {
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
            50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
            337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
            2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
            15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
        };

        auto decodeNibble = [&](int nibble, int& predictor, int& stepIndex) -> int16_t {
            int step = IMA_StepTable[stepIndex];
            int diff = step >> 3;
            if (nibble & 1) diff += step >> 2;
            if (nibble & 2) diff += step >> 1;
            if (nibble & 4) diff += step;
            if (nibble & 8) diff = -diff;
            predictor += diff;
            if (predictor > 32767) predictor = 32767;
            if (predictor < -32768) predictor = -32768;
            stepIndex += IMA_IndexTable[nibble];
            if (stepIndex < 0) stepIndex = 0;
            if (stepIndex > 88) stepIndex = 88;
            return (int16_t)predictor;
        };

        if (blockAlign == 0 || channels == 0) {
            errorOut = "Invalid Wwise ADPCM params";
            return false;
        }

        // Wwise ADPCM: block is split by channel, NOT interleaved
        // For stereo: [Left header 4B][Left data][Right header 4B][Right data]
        size_t bytesPerChannel = blockAlign / channels;
        size_t numBlocks = audioSize / blockAlign;

        pcmOut.reserve(numBlocks * (bytesPerChannel - 4) * 2 * channels);

        for (size_t blk = 0; blk < numBlocks; blk++) {
            const uint8_t* blockStart = audioData + blk * blockAlign;

            std::vector<std::vector<int16_t>> channelSamples(channels);

            for (int ch = 0; ch < channels; ch++) {
                const uint8_t* chData = blockStart + ch * bytesPerChannel;

                // 4-byte header: predictor (2), step index (1), reserved (1)
                int predictor = (int16_t)(chData[0] | (chData[1] << 8));
                int stepIndex = chData[2];
                if (stepIndex > 88) stepIndex = 88;

                // First sample is the predictor
                channelSamples[ch].push_back((int16_t)predictor);

                // Decode data bytes
                size_t dataBytes = bytesPerChannel - 4;
                for (size_t i = 0; i < dataBytes; i++) {
                    uint8_t byte = chData[4 + i];
                    channelSamples[ch].push_back(decodeNibble(byte & 0x0F, predictor, stepIndex));
                    channelSamples[ch].push_back(decodeNibble((byte >> 4) & 0x0F, predictor, stepIndex));
                }
            }

            // Interleave channels
            size_t samplesPerChannel = channelSamples[0].size();
            for (size_t s = 0; s < samplesPerChannel; s++) {
                for (int ch = 0; ch < channels; ch++) {
                    pcmOut.push_back(channelSamples[ch][s]);
                }
            }
        }
        return true;
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
    } else if (formatTag == 0x0069 || formatTag == 0x0011) {
        // Wwise IMA ADPCM
        static const int IMA_IndexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
        static const int IMA_StepTable[89] = {
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
            50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
            337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
            2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
            15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
        };

        auto decodeNibble = [&](int nibble, int& predictor, int& stepIndex) -> int16_t {
            int step = IMA_StepTable[stepIndex];
            int diff = step >> 3;
            if (nibble & 1) diff += step >> 2;
            if (nibble & 2) diff += step >> 1;
            if (nibble & 4) diff += step;
            if (nibble & 8) diff = -diff;
            predictor += diff;
            if (predictor > 32767) predictor = 32767;
            if (predictor < -32768) predictor = -32768;
            stepIndex += IMA_IndexTable[nibble];
            if (stepIndex < 0) stepIndex = 0;
            if (stepIndex > 88) stepIndex = 88;
            return (int16_t)predictor;
        };

        if (blockAlign == 0 || channels == 0) {
            errorOut = "Invalid IMA ADPCM params";
            return false;
        }

        size_t numBlocks = audioSize / blockAlign;
        pcmOut.reserve(numBlocks * blockAlign * 2);

        for (size_t blk = 0; blk < numBlocks; blk++) {
            const uint8_t* bd = audioData + blk * blockAlign;

            std::vector<int> predictor(channels), stepIndex(channels);

            // Read headers (4 bytes per channel)
            for (int ch = 0; ch < channels; ch++) {
                size_t headerOff = ch * 4;
                predictor[ch] = (int16_t)(bd[headerOff] | (bd[headerOff + 1] << 8));
                stepIndex[ch] = bd[headerOff + 2];
                if (stepIndex[ch] > 88) stepIndex[ch] = 88;
            }

            // Output initial samples
            if (channels == 1) {
                pcmOut.push_back((int16_t)predictor[0]);
            } else {
                pcmOut.push_back((int16_t)predictor[0]);
                pcmOut.push_back((int16_t)predictor[1]);
            }

            const uint8_t* data = bd + 4 * channels;
            size_t dataSize = blockAlign - 4 * channels;

            if (channels == 1) {
                for (size_t i = 0; i < dataSize; i++) {
                    uint8_t byte = data[i];
                    pcmOut.push_back(decodeNibble(byte & 0x0F, predictor[0], stepIndex[0]));
                    pcmOut.push_back(decodeNibble((byte >> 4) & 0x0F, predictor[0], stepIndex[0]));
                }
            } else {
                // Stereo: data comes in 4-byte chunks per channel, interleaved
                size_t pos = 0;
                while (pos + 8 <= dataSize) {
                    int16_t left[8], right[8];

                    // Decode 4 bytes (8 samples) from left channel
                    for (int i = 0; i < 4; i++) {
                        uint8_t byte = data[pos + i];
                        left[i * 2] = decodeNibble(byte & 0x0F, predictor[0], stepIndex[0]);
                        left[i * 2 + 1] = decodeNibble((byte >> 4) & 0x0F, predictor[0], stepIndex[0]);
                    }

                    // Decode 4 bytes (8 samples) from right channel
                    for (int i = 0; i < 4; i++) {
                        uint8_t byte = data[pos + 4 + i];
                        right[i * 2] = decodeNibble(byte & 0x0F, predictor[1], stepIndex[1]);
                        right[i * 2 + 1] = decodeNibble((byte >> 4) & 0x0F, predictor[1], stepIndex[1]);
                    }

                    // Interleave L R L R
                    for (int i = 0; i < 8; i++) {
                        pcmOut.push_back(left[i]);
                        pcmOut.push_back(right[i]);
                    }

                    pos += 8;
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

std::string AudioExport::SanitizeFilename(const std::string& name)
{
    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        if (c == '<' || c == '>' || c == ':' || c == '"' ||
            c == '/' || c == '\\' || c == '|' || c == '?' || c == '*') {
            result += '_';
        } else {
            result += c;
        }
    }
    return result;
}

std::string AudioExport::GetExportFilename(uint32_t wemId, const std::string& extension)
{
    std::string resolved = UI_ContentBrowser::GetWemDisplayName(wemId);
    if (resolved.empty() || resolved == std::to_string(wemId)) {
        return std::to_string(wemId) + extension;
    }
    return SanitizeFilename(resolved) + "_" + std::to_string(wemId) + extension;
}

bool AudioExport::ExportWEMToWAV(const uint8_t* wemData, size_t wemSize, uint32_t wemId,
                                  const std::string& outputDir, std::string& outFilepath)
{
    std::string filename = GetExportFilename(wemId, ".wav");
    outFilepath = (std::filesystem::path(outputDir) / filename).string();
    return ExportWEMToWAV(wemData, wemSize, outFilepath);
}

bool AudioExport::ExportWEMRaw(const uint8_t* wemData, size_t wemSize, uint32_t wemId,
                                const std::string& outputDir, std::string& outFilepath)
{
    std::string filename = GetExportFilename(wemId, ".wem");
    outFilepath = (std::filesystem::path(outputDir) / filename).string();

    std::ofstream file(outFilepath, std::ios::binary);
    if (!file.is_open()) {
        s_lastError = "Failed to create file: " + outFilepath;
        return false;
    }

    file.write(reinterpret_cast<const char*>(wemData), wemSize);
    s_lastError.clear();
    return true;
}

}