#include "texjpeg.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace Tex
{
    namespace Jpeg
    {
        static const uint8_t dcLuminanceLengths[] = { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
        static const uint8_t acLuminanceLengths[] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125 };
        static const uint8_t dcChrominanceLengths[] = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
        static const uint8_t acChrominanceLengths[] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119 };

        static const uint8_t dcLuminanceValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        static const uint8_t acLuminanceValues[] = {
            0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51,
            0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1,
            0x15, 0x52, 0xD1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18,
            0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57,
            0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
            0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92,
            0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xa5, 0xA6, 0xA7,
            0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
            0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
            0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEa, 0xF1, 0xF2,
            0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFa };

        static const uint8_t dcChrominanceValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        static const uint8_t acChrominanceValues[] = {
            0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07,
            0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09,
            0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25,
            0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
            0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
            0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74,
            0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
            0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xa2, 0xA3, 0xA4, 0xA5,
            0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA,
            0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
            0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2,
            0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA };

        static const uint8_t zigzagMapping[] = {
            0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42, 3, 8, 12, 17, 25, 30, 41, 43,
            9, 11, 18, 24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55,
            60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63 };

        static const uint8_t lumQuantizationTemplate[] = {
            16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55,
            14, 13, 16, 24, 40, 57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62,
            18, 22, 37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92,
            49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99 };

        static const uint8_t chromaQuantizationTemplate[] = {
            17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
            24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99 };

        struct HuffmanLookup {
            uint8_t value;
            uint8_t bits;
        };

        class BitStream {
            const uint8_t* mData;
            size_t mSize;
            size_t mBytePos;
            int mBitPos;
            uint32_t mBuffer;
            int mBufferBits;

        public:
            BitStream(const uint8_t* data, size_t size)
                : mData(data), mSize(size), mBytePos(0), mBitPos(0), mBuffer(0), mBufferBits(0) {
                Refill();
            }

            void Refill() {
                while (mBufferBits <= 24 && mBytePos < mSize) {
                    mBuffer |= ((uint32_t)mData[mBytePos++]) << (24 - mBufferBits);
                    mBufferBits += 8;
                }
            }

            uint8_t ReadBit() {
                if (mBufferBits == 0) Refill();
                uint8_t bit = (mBuffer >> 31) & 1;
                mBuffer <<= 1;
                mBufferBits--;
                return bit;
            }

            uint16_t ReadBits(int n) {
                if (n == 0) return 0;
                if (mBufferBits < n) Refill();
                uint16_t val = (uint16_t)(mBuffer >> (32 - n));
                mBuffer <<= n;
                mBufferBits -= n;
                return val;
            }

            uint16_t PeekBits(int n) {
                if (mBufferBits < n) Refill();
                return (uint16_t)(mBuffer >> (32 - n));
            }

            void SkipBits(int n) {
                mBuffer <<= n;
                mBufferBits -= n;
            }
        };

        class FastIDCT {
            static int32_t sDctTable[8][8];
            static bool sInitialized;

        public:
            static void Init() {
                if (sInitialized) return;
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        double c = (y == 0) ? 0.353553390593 : 0.5;
                        double angle = ((2 * x + 1) * y * 3.14159265358979) / 16.0;
                        sDctTable[y][x] = (int32_t)(c * cos(angle) * 256.0 + 0.5);
                    }
                }
                sInitialized = true;
            }

            static void Transform(int16_t* block) {
                int32_t temp[64];
                int32_t row[8];

                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int32_t sum = 0;
                        for (int k = 0; k < 8; k++) {
                            sum += sDctTable[k][y] * block[k * 8 + x];
                        }
                        temp[y * 8 + x] = sum;
                    }
                }

                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int32_t sum = 0;
                        for (int k = 0; k < 8; k++) {
                            sum += temp[y * 8 + k] * sDctTable[k][x];
                        }
                        block[y * 8 + x] = (int16_t)((sum + 32768) >> 16);
                    }
                }
            }
        };

        int32_t FastIDCT::sDctTable[8][8];
        bool FastIDCT::sInitialized = false;

        static inline int Clamp(int value, int minVal, int maxVal) {
            return (value < minVal) ? minVal : (value > maxVal) ? maxVal : value;
        }

        class Decoder {
            HuffmanLookup dcLumLookup[65536];
            HuffmanLookup acLumLookup[65536];
            HuffmanLookup dcChromLookup[65536];
            HuffmanLookup acChromLookup[65536];
            int16_t lumQuantTables[101][64];
            int16_t chromQuantTables[101][64];
            bool initialized = false;

            void BuildHuffmanLookup(HuffmanLookup* lookup, const uint8_t* lengths, int numLengths, const uint8_t* values) {
                memset(lookup, 0xFF, sizeof(HuffmanLookup) * 65536);
                int curIndex = 0;
                uint16_t code = 0;
                for (int length = 0; length < numLengths; length++) {
                    int numValues = lengths[length];
                    int bits = length + 1;
                    for (int i = 0; i < numValues; i++) {
                        uint16_t pattern = code << (16 - bits);
                        int count = 1 << (16 - bits);
                        for (int j = 0; j < count; j++) {
                            lookup[pattern + j].value = values[curIndex];
                            lookup[pattern + j].bits = (uint8_t)bits;
                        }
                        curIndex++;
                        code++;
                    }
                    code <<= 1;
                }
            }

            void GenerateQuantizationTable(int16_t* table, int qualityFactor, const uint8_t* templ) {
                int multiplier = 200 - qualityFactor * 2;
                for (int i = 0; i < 64; i++) {
                    table[i] = (int16_t)((templ[i] * multiplier) / 100);
                }
            }

        public:
            void Initialize() {
                if (initialized) return;
                FastIDCT::Init();
                BuildHuffmanLookup(dcLumLookup, dcLuminanceLengths, 16, dcLuminanceValues);
                BuildHuffmanLookup(acLumLookup, acLuminanceLengths, 16, acLuminanceValues);
                BuildHuffmanLookup(dcChromLookup, dcChrominanceLengths, 16, dcChrominanceValues);
                BuildHuffmanLookup(acChromLookup, acChrominanceLengths, 16, acChrominanceValues);

                for (int i = 0; i <= 100; i++) {
                    GenerateQuantizationTable(lumQuantTables[i], i, lumQuantizationTemplate);
                    GenerateQuantizationTable(chromQuantTables[i], i, chromaQuantizationTemplate);
                }
                initialized = true;
            }

            uint8_t DecodeValue(BitStream& s, const HuffmanLookup* lookup) {
                uint16_t peek = s.PeekBits(16);
                const HuffmanLookup& entry = lookup[peek];
                if (entry.bits == 0xFF) {
                    uint16_t word = 0;
                    for (int len = 1; len <= 16; len++) {
                        word = (word << 1) | s.ReadBit();
                        uint16_t pattern = word << (16 - len);
                        if (lookup[pattern].bits == len) return lookup[pattern].value;
                    }
                    return 0;
                }
                s.SkipBits(entry.bits);
                return entry.value;
            }

            int16_t Extend(uint16_t val, uint16_t len) {
                if (len == 0) return 0;
                if (val < (1u << (len - 1))) return (int16_t)(val + (-1 << len) + 1);
                return (int16_t)val;
            }

            void UnZigZag(int16_t* block) {
                int16_t buf[64];
                for (int i = 0; i < 64; i++) buf[i] = block[zigzagMapping[i]];
                memcpy(block, buf, sizeof(int16_t) * 64);
            }

            void Dequantize(int16_t* block, const int16_t* qTable) {
                for (int i = 0; i < 64; i++) block[i] = (int16_t)(block[i] * qTable[i]);
            }

            int16_t DecodeBlock(BitStream& s, int16_t* block, const HuffmanLookup* dc, const HuffmanLookup* ac, int16_t prevDc) {
                uint8_t dcLen = DecodeValue(s, dc);
                uint16_t eps = s.ReadBits(dcLen);
                int16_t delta = Extend(eps, dcLen);
                int16_t curDc = (int16_t)(delta + prevDc);
                block[0] = curDc;

                for (int idx = 1; idx < 64;) {
                    uint8_t acVal = DecodeValue(s, ac);
                    if (acVal == 0) break;
                    if (acVal == 0xF0) { idx += 16; continue; }
                    idx += (acVal >> 4) & 0xF;
                    uint8_t acLen = acVal & 0xF;
                    eps = s.ReadBits(acLen);
                    if (idx < 64) block[idx] = Extend(eps, acLen);
                    idx++;
                }
                return curDc;
            }

            int16_t ProcessBlock(int16_t prevDc, BitStream& s, const HuffmanLookup* dc, const HuffmanLookup* ac,
                const int16_t* q, bool isLum, int16_t* out) {
                int16_t buf[64] = { 0 };
                int16_t curDc = DecodeBlock(s, buf, dc, ac, prevDc);
                UnZigZag(buf);
                Dequantize(buf, q);
                FastIDCT::Transform(buf);
                for (int i = 0; i < 64; i++) {
                    int val = buf[i];
                    if (isLum) val = Clamp(val + 128, 0, 255);
                    else val = Clamp(val, -256, 255);
                    out[i] = (int16_t)val;
                }
                return curDc;
            }

            void YCbCrToColor32(int y, int lum1, int cb, int cr, uint8_t* out) {
                int a = y - (cr >> 1);
                out[0] = (uint8_t)Clamp(a - (cb >> 1), 0, 255);
                out[1] = (uint8_t)Clamp(a + cr, 0, 255);
                out[2] = (uint8_t)Clamp(out[0] + cb, 0, 255);
                out[3] = (uint8_t)Clamp(lum1, 0, 255);
            }

            void DecompressFormat0(int w, int h, BitStream& s, const int16_t* lq, const int16_t* cq, std::vector<uint8_t>& data) {
                int16_t l0[4][64], l1[4][64], c0[64], c1[64];
                int16_t prevDc[4] = { 0 };
                int mipW = ((w + 15) / 16) * 16;
                int mipH = ((h + 15) / 16) * 16;
                data.resize(mipW * mipH * 4);

                for (int y = 0; y < mipH / 16; y++) {
                    for (int x = 0; x < mipW / 16; x++) {
                        for (int i = 0; i < 4; i++) prevDc[0] = ProcessBlock(prevDc[0], s, dcLumLookup, acLumLookup, lq, true, l0[i]);
                        prevDc[1] = ProcessBlock(prevDc[1], s, dcChromLookup, acChromLookup, cq + 64, false, c0);
                        prevDc[2] = ProcessBlock(prevDc[2], s, dcChromLookup, acChromLookup, cq + 128, false, c1);
                        for (int i = 0; i < 4; i++) prevDc[3] = ProcessBlock(prevDc[3], s, dcLumLookup, acLumLookup, lq + 192, true, l1[i]);

                        for (int r = 0; r < 16; r++) {
                            if (y * 16 + r >= h) continue;
                            for (int c = 0; c < 16; c++) {
                                if (x * 16 + c >= w) continue;
                                int blk = ((r >= 8) ? 2 : 0) + ((c >= 8) ? 1 : 0);
                                int lIdx = (r % 8) * 8 + (c % 8);
                                int cIdx = (r / 2) * 8 + (c / 2);
                                int pIdx = ((y * 16 + r) * w + (x * 16 + c)) * 4;
                                YCbCrToColor32(l0[blk][lIdx], l1[blk][lIdx], c0[cIdx], c1[cIdx], &data[pIdx]);
                            }
                        }
                    }
                }
            }

            void DecompressFormat1(int w, int h, BitStream& s, const Header& hdr, const int16_t* lq, std::vector<uint8_t>& data) {
                int16_t l[4][64];
                int16_t prevDc[4] = { 0 };
                int mipW = ((w + 7) / 8) * 8;
                int mipH = ((h + 7) / 8) * 8;
                data.resize(mipW * mipH * 4);

                for (int y = 0; y < mipH / 8; y++) {
                    for (int x = 0; x < mipW / 8; x++) {
                        prevDc[0] = ProcessBlock(prevDc[0], s, dcLumLookup, acLumLookup, lq, true, l[0]);
                        prevDc[1] = ProcessBlock(prevDc[1], s, dcLumLookup, acLumLookup, lq + 64, true, l[1]);
                        if (hdr.layerInfos[2].hasReplacement == 0)
                            prevDc[2] = ProcessBlock(prevDc[2], s, dcLumLookup, acLumLookup, lq + 128, true, l[2]);
                        if (hdr.layerInfos[3].hasReplacement == 0)
                            prevDc[3] = ProcessBlock(prevDc[3], s, dcLumLookup, acLumLookup, lq + 192, true, l[3]);

                        for (int iy = 0; iy < 8; iy++) {
                            for (int ix = 0; ix < 8; ix++) {
                                int idx = iy * 8 + ix;
                                int pIdx = ((y * 8 + iy) * w + (x * 8 + ix)) * 4;
                                data[pIdx + 0] = (uint8_t)l[2][idx];
                                data[pIdx + 1] = (uint8_t)l[1][idx];
                                data[pIdx + 2] = (uint8_t)l[3][idx];
                                data[pIdx + 3] = (uint8_t)l[0][idx];
                            }
                        }
                    }
                }
            }

            void DecompressFormat2(int w, int h, BitStream& s, const Header& hdr, const int16_t* lq, const int16_t* cq, std::vector<uint8_t>& data) {
                int16_t l0[64], l1[64], c0[64], c1[64];
                int16_t prevDc[4] = { 0 };
                int mipW = ((w + 7) / 8) * 8;
                int mipH = ((h + 7) / 8) * 8;
                data.resize(mipW * mipH * 4);

                for (int y = 0; y < mipH / 8; y++) {
                    for (int x = 0; x < mipW / 8; x++) {
                        prevDc[0] = ProcessBlock(prevDc[0], s, dcLumLookup, acLumLookup, lq, true, l0);
                        prevDc[1] = ProcessBlock(prevDc[1], s, dcChromLookup, acChromLookup, cq + 64, false, c0);
                        prevDc[2] = ProcessBlock(prevDc[2], s, dcChromLookup, acChromLookup, cq + 128, false, c1);

                        bool hasL1 = (hdr.layerInfos[3].hasReplacement == 0);
                        if (hasL1)
                            prevDc[3] = ProcessBlock(prevDc[3], s, dcLumLookup, acLumLookup, lq + 192, true, l1);

                        for (int r = 0; r < 8; r++) {
                            if (y * 8 + r >= h) continue;
                            for (int c = 0; c < 8; c++) {
                                if (x * 8 + c >= w) continue;
                                int idx = r * 8 + c;
                                int16_t alpha = hasL1 ? l1[idx] : 255;
                                int pIdx = ((y * 8 + r) * w + (x * 8 + c)) * 4;
                                YCbCrToColor32(l0[idx], alpha, c0[idx], c1[idx], &data[pIdx]);
                            }
                        }
                    }
                }
            }

            bool Decompress(const Header& header, int mipLevel, const std::vector<uint8_t>& input, std::vector<uint8_t>& output, int& outW, int& outH) {
                Initialize();

                int16_t lq[256], cq[256];
                for (int i = 0; i < 4; i++) {
                    int q = header.layerInfos[i].quality;
                    if (q > 100) return false;
                    memcpy(lq + i * 64, lumQuantTables[q], sizeof(int16_t) * 64);
                    memcpy(cq + i * 64, chromQuantTables[q], sizeof(int16_t) * 64);
                }

                BitStream bs(input.data(), input.size());
                int w = (int)(header.width / std::pow(2.0, (double)mipLevel));
                int h = (int)(header.height / std::pow(2.0, (double)mipLevel));
                if (w < 1) w = 1;
                if (h < 1) h = 1;
                outW = w; outH = h;

                std::vector<uint8_t> data;
                switch (header.compressionFormat) {
                case 0: DecompressFormat0(w, h, bs, lq, cq, data); break;
                case 1: DecompressFormat1(w, h, bs, header, lq, data); break;
                case 2: DecompressFormat2(w, h, bs, header, lq, cq, data); break;
                default: return false;
                }

                output = std::move(data);
                for (size_t i = 0; i < output.size(); i += 4) {
                    uint8_t r = output[i];
                    uint8_t b = output[i + 2];
                    output[i] = b;
                    output[i + 2] = r;
                }
                return true;
            }
        };

        bool Decode(const Header& header, int mipLevel, const std::vector<uint8_t>& data, std::vector<uint8_t>& outRGBA, int& outW, int& outH) {
            static Decoder decoder;
            return decoder.Decompress(header, mipLevel, data, outRGBA, outW, outH);
        }
    }
}