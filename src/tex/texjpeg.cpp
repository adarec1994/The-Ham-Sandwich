#include "texjpeg.h"
#include <cmath>
#include <map>
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

        class BitStream
        {
            const uint8_t* mData;
            size_t mSize;
            size_t mBitPos;

        public:
            BitStream(const uint8_t* data, size_t size) : mData(data), mSize(size), mBitPos(0) {}

            uint8_t ReadBit()
            {
                size_t bytePos = mBitPos / 8;
                if (bytePos >= mSize) return 0;
                size_t bitPos = 7 - (mBitPos % 8);
                uint8_t ret = (mData[bytePos] >> bitPos) & 1;
                mBitPos++;
                return ret;
            }

            uint16_t ReadBits(int numBits)
            {
                uint16_t ret = 0;
                for (int i = 0; i < numBits; ++i)
                {
                    ret <<= 1;
                    ret |= ReadBit();
                }
                return ret;
            }
        };

        struct IDCT2
        {
            static void DoIdct(int16_t* m)
            {
                static bool tablesGenerated = false;
                static float Dct[64];
                static float DctT[64];

                if (!tablesGenerated)
                {
                    for (int y = 0, o = 0; y < 8; y++)
                    {
                        for (int x = 0; x < 8; x++)
                        {
                            Dct[o++] = (float)(std::sqrt(y == 0 ? 0.125f : 0.250f) * std::cos(((2 * x + 1) * y * 3.1415926535f) * 0.0625f));
                        }
                    }
                    for (int y = 0; y < 8; y++)
                        for (int x = 0; x < 8; x++)
                            DctT[x * 8 + y] = Dct[y * 8 + x];
                    tablesGenerated = true;
                }

                float source[64];
                for (int i = 0; i < 64; i++) source[i] = (float)m[i];

                float temp[64];
                for (int y = 0; y < 8; y++)
                {
                    for (int x = 0; x < 8; x++)
                    {
                        float sum = 0;
                        for (int k = 0; k < 8; k++) sum += DctT[y * 8 + k] * source[k * 8 + x];
                        temp[y * 8 + x] = sum;
                    }
                }

                for (int y = 0; y < 8; y++)
                {
                    for (int x = 0; x < 8; x++)
                    {
                        float sum = 0;
                        for (int k = 0; k < 8; k++) sum += temp[y * 8 + k] * Dct[k * 8 + x];
                        source[y * 8 + x] = sum;
                    }
                }

                for (int i = 0; i < 64; i++) m[i] = (int16_t)std::round(source[i]);
            }
        };

        static int Clamp(int value, int min, int max)
        {
            return (value < min) ? min : (value > max) ? max : value;
        }

        class Decoder
        {
            using HuffmanTable = std::map<uint64_t, uint8_t>;
            HuffmanTable dcLum, acLum, dcChrom, acChrom;
            std::vector<float> lumQuantTables[101];
            std::vector<float> chromQuantTables[101];
            bool initialized = false;

            HuffmanTable GenerateHuffmanTable(const uint8_t* lengths, int numLengths, const uint8_t* values)
            {
                HuffmanTable table;
                int curIndex = 0;
                uint16_t code = 0;
                for (int length = 0; length < numLengths; length++)
                {
                    int numValues = lengths[length];
                    for (int i = 0; i < numValues; i++)
                    {
                        uint64_t mixed = ((uint64_t)(length + 1) << 32) | code;
                        table[mixed] = values[curIndex++];
                        code++;
                    }
                    code <<= 1;
                }
                return table;
            }

            std::vector<float> GenerateQuantizationTable(int qualityFactor, const uint8_t* templ)
            {
                std::vector<float> table(64);
                float multiplier = (200.0f - qualityFactor * 2.0f) * 0.01f;
                for (int i = 0; i < 64; i++) table[i] = templ[i] * multiplier;
                return table;
            }

        public:
            void Initialize()
            {
                if (initialized) return;
                dcLum = GenerateHuffmanTable(dcLuminanceLengths, 16, dcLuminanceValues);
                acLum = GenerateHuffmanTable(acLuminanceLengths, 16, acLuminanceValues);
                dcChrom = GenerateHuffmanTable(dcChrominanceLengths, 16, dcChrominanceValues);
                acChrom = GenerateHuffmanTable(acChrominanceLengths, 16, acChrominanceValues);

                for (int i = 0; i <= 100; i++)
                {
                    lumQuantTables[i] = GenerateQuantizationTable(i, lumQuantizationTemplate);
                    chromQuantTables[i] = GenerateQuantizationTable(i, chromaQuantizationTemplate);
                }
                initialized = true;
            }

            uint8_t DecodeValue(BitStream& s, const HuffmanTable& table)
            {
                uint16_t word = 0;
                uint8_t len = 0;
                do
                {
                    word <<= 1;
                    word |= s.ReadBit();
                    len++;
                    uint64_t key = ((uint64_t)len << 32) | word;
                    auto it = table.find(key);
                    if (it != table.end()) return it->second;
                } while (len < 16);
                return 0;
            }

            int16_t Extend(uint16_t val, uint16_t len)
            {
                if (val < (1 << (len - 1))) return (int16_t)(val + (-1 << len) + 1);
                return (int16_t)val;
            }

            void UnZigZag(int16_t* block)
            {
                int16_t buf[64];
                for (int i = 0; i < 64; i++) buf[i] = block[zigzagMapping[i]];
                std::memcpy(block, buf, sizeof(int16_t) * 64);
            }

            void Dequantize(int16_t* block, const std::vector<float>& qTable)
            {
                for (int i = 0; i < 64; i++) block[i] = (int16_t)(block[i] * qTable[i]);
            }

            int16_t DecodeBlock(BitStream& s, int16_t* block, const HuffmanTable& dc, const HuffmanTable& ac, int16_t prevDc)
            {
                uint8_t dcLen = DecodeValue(s, dc);
                uint16_t eps = s.ReadBits(dcLen);
                int16_t delta = Extend(eps, dcLen);
                int16_t curDc = (int16_t)(delta + prevDc);
                block[0] = curDc;

                for (int idx = 1; idx < 64;)
                {
                    uint8_t acVal = DecodeValue(s, ac);
                    if (acVal == 0) break;
                    if (acVal == 0xF0) { idx += 16; continue; }
                    idx += (acVal >> 4) & 0xF;
                    uint8_t acLen = acVal & 0xF;
                    eps = s.ReadBits(acLen);
                    block[idx] = Extend(eps, acLen);
                    idx++;
                }
                return curDc;
            }

            int16_t ProcessBlock(int16_t prevDc, BitStream& s, const HuffmanTable& dc, const HuffmanTable& ac, 
                const std::vector<float>& q, bool isLum, int16_t* out)
            {
                int16_t buf[64] = { 0 };
                int16_t curDc = DecodeBlock(s, buf, dc, ac, prevDc);
                UnZigZag(buf);
                Dequantize(buf, q);
                IDCT2::DoIdct(buf);
                for (int i = 0; i < 64; i++)
                {
                    int val = buf[i];
                    if (isLum) val = Clamp(val + 128, 0, 255);
                    else val = Clamp(val, -256, 255);
                    out[i] = (int16_t)val;
                }
                return curDc;
            }

            std::vector<uint8_t> YCbCrToColor32(int y, int lum1, int cb, int cr)
            {
                int a = y - (cr >> 1);
                uint8_t beta = (uint8_t)Clamp(a + cr, 0, 0xFF);
                uint8_t gamma = (uint8_t)Clamp(a - (cb >> 1), 0, 0xFF);
                uint8_t delta = (uint8_t)Clamp(gamma + cb, 0, 0xFF);
                uint8_t alpha = (uint8_t)Clamp(lum1, 0, 0xFF);
                return { gamma, beta, delta, alpha };
            }

            void DecompressFormat0(int w, int h, BitStream& s, const std::vector<float>* lq, const std::vector<float>* cq, std::vector<uint8_t>& data)
            {
                int16_t l0[4][64], l1[4][64], c0[64], c1[64];
                int16_t prevDc[4] = { 0 };
                int mipW = ((w + 15) / 16) * 16;
                int mipH = ((h + 15) / 16) * 16;
                data.resize(mipW * mipH * 4);

                for (int y = 0; y < mipH / 16; y++)
                {
                    for (int x = 0; x < mipW / 16; x++)
                    {
                        for (int i = 0; i < 4; i++) prevDc[0] = ProcessBlock(prevDc[0], s, dcLum, acLum, lq[0], true, l0[i]);
                        prevDc[1] = ProcessBlock(prevDc[1], s, dcChrom, acChrom, cq[1], false, c0);
                        prevDc[2] = ProcessBlock(prevDc[2], s, dcChrom, acChrom, cq[2], false, c1);
                        for (int i = 0; i < 4; i++) prevDc[3] = ProcessBlock(prevDc[3], s, dcLum, acLum, lq[3], true, l1[i]);

                        for (int r = 0; r < 16; r++)
                        {
                            if (y * 16 + r >= h) continue;
                            for (int c = 0; c < 16; c++)
                            {
                                if (x * 16 + c >= w) continue;
                                int blk = ((r >= 8) ? 2 : 0) + ((c >= 8) ? 1 : 0);
                                int lIdx = (r % 8) * 8 + (c % 8);
                                int cIdx = (r / 2) * 8 + (c / 2);
                                auto color = YCbCrToColor32(l0[blk][lIdx], l1[blk][lIdx], c0[cIdx], c1[cIdx]);
                                int pIdx = ((y * 16 + r) * w + (x * 16 + c)) * 4;
                                data[pIdx + 0] = color[0]; data[pIdx + 1] = color[1];
                                data[pIdx + 2] = color[2]; data[pIdx + 3] = color[3];
                            }
                        }
                    }
                }
            }

            void DecompressFormat1(int w, int h, BitStream& s, const Header& hdr, const std::vector<float>* lq, std::vector<uint8_t>& data)
            {
                int16_t l[4][64];
                int16_t prevDc[4] = { 0 };
                int mipW = ((w + 7) / 8) * 8;
                int mipH = ((h + 7) / 8) * 8;
                data.resize(mipW * mipH * 4);

                for (int y = 0; y < mipH / 8; y++)
                {
                    for (int x = 0; x < mipW / 8; x++)
                    {
                        prevDc[0] = ProcessBlock(prevDc[0], s, dcLum, acLum, lq[0], true, l[0]);
                        prevDc[1] = ProcessBlock(prevDc[1], s, dcLum, acLum, lq[1], true, l[1]);
                        if (hdr.layerInfos[2].hasReplacement == 0)
                            prevDc[2] = ProcessBlock(prevDc[2], s, dcLum, acLum, lq[2], true, l[2]);
                        if (hdr.layerInfos[3].hasReplacement == 0)
                            prevDc[3] = ProcessBlock(prevDc[3], s, dcLum, acLum, lq[3], true, l[3]);

                        for (int iy = 0; iy < 8; iy++)
                        {
                            for (int ix = 0; ix < 8; ix++)
                            {
                                int idx = iy * 8 + ix;
                                uint8_t r = (uint8_t)l[0][idx];
                                uint8_t g = (uint8_t)l[1][idx];
                                uint8_t b = 255;
                                if (hdr.layerInfos[2].hasReplacement == 0) b = (uint8_t)l[2][idx];
                                uint8_t a = 255;
                                if (hdr.layerInfos[3].hasReplacement == 0) a = (uint8_t)l[3][idx];
                                
                                int pIdx = ((y * 8 + iy) * w + (x * 8 + ix)) * 4;
                                data[pIdx + 0] = b; data[pIdx + 1] = g;
                                data[pIdx + 2] = a; data[pIdx + 3] = r;
                            }
                        }
                    }
                }
            }

            void DecompressFormat2(int w, int h, BitStream& s, const Header& hdr, const std::vector<float>* lq, const std::vector<float>* cq, std::vector<uint8_t>& data)
            {
                int16_t l0[64], l1[64], c0[64], c1[64];
                int16_t prevDc[4] = { 0 };
                int mipW = ((w + 7) / 8) * 8;
                int mipH = ((h + 7) / 8) * 8;
                data.resize(mipW * mipH * 4);

                for (int y = 0; y < mipH / 8; y++)
                {
                    for (int x = 0; x < mipW / 8; x++)
                    {
                        prevDc[0] = ProcessBlock(prevDc[0], s, dcLum, acLum, lq[0], true, l0);
                        prevDc[1] = ProcessBlock(prevDc[1], s, dcChrom, acChrom, cq[1], false, c0);
                        prevDc[2] = ProcessBlock(prevDc[2], s, dcChrom, acChrom, cq[2], false, c1);
                        if (hdr.layerInfos[3].hasReplacement == 0)
                            prevDc[3] = ProcessBlock(prevDc[3], s, dcLum, acLum, lq[3], true, l1);
                        else
                            std::memset(l1, 1, sizeof(l1));

                        for (int r = 0; r < 8; r++)
                        {
                            if (y * 8 + r >= h) continue;
                            for (int c = 0; c < 8; c++)
                            {
                                if (x * 8 + c >= w) continue;
                                int idx = r * 8 + c;
                                auto color = YCbCrToColor32(l0[idx], l1[idx], c0[idx], c1[idx]);
                                int pIdx = ((y * 8 + r) * w + (x * 8 + c)) * 4;
                                data[pIdx + 0] = color[0]; data[pIdx + 1] = color[1];
                                data[pIdx + 2] = color[2]; data[pIdx + 3] = color[3];
                            }
                        }
                    }
                }
            }

            bool Decompress(const Header& header, int mipLevel, const std::vector<uint8_t>& input, std::vector<uint8_t>& output, int& outW, int& outH)
            {
                Initialize();
                std::vector<float> lq[4], cq[4];
                for (int i = 0; i < 4; i++)
                {
                    int q = header.layerInfos[i].quality;
                    if (q > 100) return false;
                    lq[i] = lumQuantTables[q];
                    cq[i] = chromQuantTables[q];
                }

                BitStream bs(input.data(), input.size());
                int w = (int)(header.width / std::pow(2, mipLevel));
                int h = (int)(header.height / std::pow(2, mipLevel));
                outW = w; outH = h;

                std::vector<uint8_t> data;
                switch (header.compressionFormat)
                {
                case 0: DecompressFormat0(w, h, bs, lq, cq, data); break;
                case 1: DecompressFormat1(w, h, bs, header, lq, data); break;
                case 2: DecompressFormat2(w, h, bs, header, lq, cq, data); break;
                default: return false;
                }

                output = data;
                for (size_t i = 0; i < output.size(); i += 4)
                {
                    uint8_t r = output[i];
                    uint8_t b = output[i + 2];
                    output[i] = b;
                    output[i + 2] = r;
                }
                return true;
            }
        };

        bool Decode(const Header& header, int mipLevel, const std::vector<uint8_t>& data, std::vector<uint8_t>& outRGBA, int& outW, int& outH)
        {
            static Decoder decoder;
            return decoder.Decompress(header, mipLevel, data, outRGBA, outW, outH);
        }
    }
}