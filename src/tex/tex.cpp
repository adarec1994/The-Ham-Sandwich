#include "tex.h"
#include "texjpeg.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <fstream>
#include "../UI/UI.h"
#include "../Archive.h"
#include "imgui.h"
#include <stb_image_write.h>

extern ID3D11Device* gDevice;
extern ID3D11DeviceContext* gContext;

static inline uint32_t rd_u32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
static inline uint16_t rd_u16(const uint8_t* p) { uint16_t v; std::memcpy(&v, p, 2); return v; }

static bool read_bytes(const uint8_t* data, size_t size, size_t& off, void* dst, size_t n)
{
    if (off + n > size) return false;
    std::memcpy(dst, data + off, n);
    off += n;
    return true;
}

namespace Tex
{

    static ID3D11Device* sDevice = nullptr;
    static ID3D11DeviceContext* sContext = nullptr;

    void SetDevice(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        sDevice = device;
        sContext = context;
    }
    static ID3D11Device* GetDevice()
    {
        if (sDevice) return sDevice;
        return gDevice;
    }

    void PreviewState::clear()
    {
        textureSRV.Reset();
        texW = texH = 0;
        hasTexture = false;
        title.clear();
        open = false;
        ownsTexture = true;

        showR = true;
        showG = true;
        showB = true;
        showA = true;
    }

    bool Header::read(const uint8_t* data, size_t size, size_t& offset)
    {
        if (size < 4) return false;

        int32_t magic = 0;
        if (!read_bytes(data, size, offset, &magic, 4)) return false;
        if (magic != 0x00474658) return false;

        if (!read_bytes(data, size, offset, &version, 4)) return false;
        if (!read_bytes(data, size, offset, &width, 4)) return false;
        if (!read_bytes(data, size, offset, &height, 4)) return false;
        if (!read_bytes(data, size, offset, &depth, 4)) return false;
        if (!read_bytes(data, size, offset, &sides, 4)) return false;
        if (!read_bytes(data, size, offset, &mipCount, 4)) return false;
        if (!read_bytes(data, size, offset, &format, 4)) return false;

        if (version > 0)
        {
            if (!read_bytes(data, size, offset, &isCompressed, 4)) return false;
            if (!read_bytes(data, size, offset, &compressionFormat, 4)) return false;

            for (int i = 0; i < 4; ++i)
            {
                if (!read_bytes(data, size, offset, &layerInfos[i].quality, 1)) return false;
                if (!read_bytes(data, size, offset, &layerInfos[i].hasReplacement, 1)) return false;
                if (!read_bytes(data, size, offset, &layerInfos[i].replacement, 1)) return false;
            }

            if (!read_bytes(data, size, offset, &imageSizesCount, 4)) return false;

            uint32_t tmp[13]{};
            for (int i = 0; i < 13; ++i)
            {
                if (!read_bytes(data, size, offset, &tmp[i], 4)) return false;
                imageSizes[i] = tmp[i];
            }

            if (!read_bytes(data, size, offset, &unk, 4)) return false;
        }
        else
        {
            isCompressed = 0;
            compressionFormat = 0;
            imageSizesCount = 0;
            imageSizes.fill(0);
            unk = 0;
        }

        textureType = TextureType::Unknown;
        switch (format)
        {
        case 0:
            if (version > 0 && isCompressed == 1)
            {
                switch (compressionFormat)
                {
                case 0: textureType = TextureType::Jpeg1; break;
                case 1: textureType = TextureType::Jpeg2; break;
                case 2: textureType = TextureType::Jpeg3; break;
                default: break;
                }
            }
            else
            {
                textureType = TextureType::Argb1;
            }
            break;
        case 1:  textureType = TextureType::Argb2; break;
        case 5:  textureType = TextureType::Argb16; break;
        case 6:  textureType = TextureType::Grayscale; break;
        case 13: textureType = TextureType::DXT1; break;
        case 14: textureType = TextureType::DXT3; break;
        case 15: textureType = TextureType::DXT5; break;
        case 18: textureType = TextureType::Garbage; break;
        default: break;
        }

        return true;
    }

    std::vector<int> File::calculateDXTSizes(int mipLevels, int width, int height, int blockSize)
    {
        std::vector<int> sizes;
        sizes.resize(std::max(0, mipLevels));
        int increment = 0;
        for (int m = mipLevels - 1; m >= 0; --m)
        {
            int w = (int)(width / std::pow(2.0, (double)m));
            int h = (int)(height / std::pow(2.0, (double)m));
            if (w < 1) w = 1;
            if (h < 1) h = 1;
            sizes[increment] = (int)(((w + 3) / 4) * ((h + 3) / 4) * blockSize);
            increment++;
        }
        return sizes;
    }

    static inline void rgb565_to_rgb8(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b)
    {
        r = (uint8_t)(((c >> 11) & 31) * 255 / 31);
        g = (uint8_t)(((c >> 5) & 63) * 255 / 63);
        b = (uint8_t)((c & 31) * 255 / 31);
    }

    bool File::decodeDXT1(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;
        outRGBA.assign((size_t)width * (size_t)height * 4, 0);

        int blocksX = (width + 3) / 4;
        int blocksY = (height + 3) / 4;
        const uint8_t* p = src;

        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                uint16_t c0 = (uint16_t)(p[0] | (p[1] << 8));
                uint16_t c1 = (uint16_t)(p[2] | (p[3] << 8));
                uint32_t codes = rd_u32(p + 4);
                p += 8;

                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565_to_rgb8(c0, r0, g0, b0);
                rgb565_to_rgb8(c1, r1, g1, b1);

                uint8_t pal[4][4]{};
                pal[0][0] = r0; pal[0][1] = g0; pal[0][2] = b0; pal[0][3] = 255;
                pal[1][0] = r1; pal[1][1] = g1; pal[1][2] = b1; pal[1][3] = 255;

                if (c0 > c1)
                {
                    pal[2][0] = (uint8_t)((2 * r0 + r1) / 3);
                    pal[2][1] = (uint8_t)((2 * g0 + g1) / 3);
                    pal[2][2] = (uint8_t)((2 * b0 + b1) / 3);
                    pal[2][3] = 255;
                    pal[3][0] = (uint8_t)((r0 + 2 * r1) / 3);
                    pal[3][1] = (uint8_t)((g0 + 2 * g1) / 3);
                    pal[3][2] = (uint8_t)((b0 + 2 * b1) / 3);
                    pal[3][3] = 255;
                }
                else
                {
                    pal[2][0] = (uint8_t)((r0 + r1) / 2);
                    pal[2][1] = (uint8_t)((g0 + g1) / 2);
                    pal[2][2] = (uint8_t)((b0 + b1) / 2);
                    pal[2][3] = 255;
                    pal[3][3] = 255;
                }

                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
                    {
                        int x = bx * 4 + px;
                        int y = by * 4 + py;
                        if (x >= width || y >= height) continue;
                        uint32_t idx = (codes >> (2 * (py * 4 + px))) & 0x3;
                        size_t o = ((size_t)y * (size_t)width + (size_t)x) * 4;
                        outRGBA[o + 0] = pal[idx][0];
                        outRGBA[o + 1] = pal[idx][1];
                        outRGBA[o + 2] = pal[idx][2];
                        outRGBA[o + 3] = pal[idx][3];
                    }
                }
            }
        }
        return true;
    }

    bool File::decodeDXT3(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;
        outRGBA.assign((size_t)width * (size_t)height * 4, 0);

        int blocksX = (width + 3) / 4;
        int blocksY = (height + 3) / 4;
        const uint8_t* p = src;

        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                const uint8_t* alpha = p;
                uint16_t c0 = (uint16_t)(p[8] | (p[9] << 8));
                uint16_t c1 = (uint16_t)(p[10] | (p[11] << 8));
                uint32_t codes = rd_u32(p + 12);
                p += 16;

                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565_to_rgb8(c0, r0, g0, b0);
                rgb565_to_rgb8(c1, r1, g1, b1);

                uint8_t pal[4][3]{};
                pal[0][0] = r0; pal[0][1] = g0; pal[0][2] = b0;
                pal[1][0] = r1; pal[1][1] = g1; pal[1][2] = b1;
                pal[2][0] = (uint8_t)((2 * r0 + r1) / 3);
                pal[2][1] = (uint8_t)((2 * g0 + g1) / 3);
                pal[2][2] = (uint8_t)((2 * b0 + b1) / 3);
                pal[3][0] = (uint8_t)((r0 + 2 * r1) / 3);
                pal[3][1] = (uint8_t)((g0 + 2 * g1) / 3);
                pal[3][2] = (uint8_t)((b0 + 2 * b1) / 3);

                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
                    {
                        int x = bx * 4 + px;
                        int y = by * 4 + py;
                        if (x >= width || y >= height) continue;

                        int pIdx = py * 4 + px;
                        int bIdx = pIdx / 2;
                        int sVal = (pIdx % 2) * 4;
                        uint8_t a = (alpha[bIdx] >> sVal) & 0x0F;
                        a = (a << 4) | a;

                        uint32_t cidx = (codes >> (2 * pIdx)) & 0x3;
                        size_t o = ((size_t)y * (size_t)width + (size_t)x) * 4;
                        outRGBA[o + 0] = pal[cidx][0];
                        outRGBA[o + 1] = pal[cidx][1];
                        outRGBA[o + 2] = pal[cidx][2];
                        outRGBA[o + 3] = a;
                    }
                }
            }
        }
        return true;
    }

    bool File::decodeDXT5(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;
        outRGBA.assign((size_t)width * (size_t)height * 4, 0);

        int blocksX = (width + 3) / 4;
        int blocksY = (height + 3) / 4;
        const uint8_t* p = src;

        for (int by = 0; by < blocksY; ++by)
        {
            for (int bx = 0; bx < blocksX; ++bx)
            {
                uint8_t a0 = p[0];
                uint8_t a1 = p[1];
                uint64_t alphaBits = 0;
                alphaBits |= (uint64_t)p[2] << 0;
                alphaBits |= (uint64_t)p[3] << 8;
                alphaBits |= (uint64_t)p[4] << 16;
                alphaBits |= (uint64_t)p[5] << 24;
                alphaBits |= (uint64_t)p[6] << 32;
                alphaBits |= (uint64_t)p[7] << 40;

                uint16_t c0 = (uint16_t)(p[8] | (p[9] << 8));
                uint16_t c1 = (uint16_t)(p[10] | (p[11] << 8));
                uint32_t codes = rd_u32(p + 12);
                p += 16;

                uint8_t alut[8]{};
                alut[0] = a0;
                alut[1] = a1;
                if (a0 > a1)
                {
                    alut[2] = (uint8_t)((6 * a0 + 1 * a1) / 7);
                    alut[3] = (uint8_t)((5 * a0 + 2 * a1) / 7);
                    alut[4] = (uint8_t)((4 * a0 + 3 * a1) / 7);
                    alut[5] = (uint8_t)((3 * a0 + 4 * a1) / 7);
                    alut[6] = (uint8_t)((2 * a0 + 5 * a1) / 7);
                    alut[7] = (uint8_t)((1 * a0 + 6 * a1) / 7);
                }
                else
                {
                    alut[2] = (uint8_t)((4 * a0 + 1 * a1) / 5);
                    alut[3] = (uint8_t)((3 * a0 + 2 * a1) / 5);
                    alut[4] = (uint8_t)((2 * a0 + 3 * a1) / 5);
                    alut[5] = (uint8_t)((1 * a0 + 4 * a1) / 5);
                    alut[6] = 0;
                    alut[7] = 255;
                }

                uint8_t r0, g0, b0, r1, g1, b1;
                rgb565_to_rgb8(c0, r0, g0, b0);
                rgb565_to_rgb8(c1, r1, g1, b1);

                uint8_t pal[4][3]{};
                pal[0][0] = r0; pal[0][1] = g0; pal[0][2] = b0;
                pal[1][0] = r1; pal[1][1] = g1; pal[1][2] = b1;
                pal[2][0] = (uint8_t)((2 * r0 + r1) / 3);
                pal[2][1] = (uint8_t)((2 * g0 + g1) / 3);
                pal[2][2] = (uint8_t)((2 * b0 + b1) / 3);
                pal[3][0] = (uint8_t)((r0 + 2 * r1) / 3);
                pal[3][1] = (uint8_t)((g0 + 2 * g1) / 3);
                pal[3][2] = (uint8_t)((b0 + 2 * b1) / 3);

                for (int py = 0; py < 4; ++py)
                {
                    for (int px = 0; px < 4; ++px)
                    {
                        int x = bx * 4 + px;
                        int y = by * 4 + py;
                        if (x >= width || y >= height) continue;

                        uint32_t cidx = (codes >> (2 * (py * 4 + px))) & 0x3;
                        uint32_t aidx = (uint32_t)((alphaBits >> (3 * (py * 4 + px))) & 0x7);

                        size_t o = ((size_t)y * (size_t)width + (size_t)x) * 4;
                        outRGBA[o + 0] = pal[cidx][0];
                        outRGBA[o + 1] = pal[cidx][1];
                        outRGBA[o + 2] = pal[cidx][2];
                        outRGBA[o + 3] = alut[aidx];
                    }
                }
            }
        }
        return true;
    }

    bool File::decodeArgb16(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;
        outRGBA.assign((size_t)width * (size_t)height * 4, 0);

        const uint8_t* p = src;
        for (int i = 0; i < width * height; ++i)
        {
            uint8_t bg = p[0];
            uint8_t ra = p[1];
            p += 2;

            uint8_t r = (uint8_t)(((ra & 0xF0) >> 4) | (ra & 0xF0));
            uint8_t g = (uint8_t)((bg & 0x0F) | ((bg & 0x0F) << 4));
            uint8_t b = (uint8_t)(((bg & 0xF0) >> 4) | (bg & 0xF0));
            uint8_t a = (uint8_t)((ra & 0x0F) | ((ra & 0x0F) << 4));

            outRGBA[i * 4 + 0] = r;
            outRGBA[i * 4 + 1] = g;
            outRGBA[i * 4 + 2] = b;
            outRGBA[i * 4 + 3] = a;
        }
        return true;
    }

    bool File::decodeGrayscale(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;
        outRGBA.assign((size_t)width * (size_t)height * 4, 0);
        int padding = (4 - (width % 4)) % 4;

        const uint8_t* p = src;
        for (int i = 0; i < width * height; ++i)
        {
            uint8_t val = *p++;
            outRGBA[i * 4 + 0] = val;
            outRGBA[i * 4 + 1] = val;
            outRGBA[i * 4 + 2] = val;
            outRGBA[i * 4 + 3] = 255;

            if ((i + 1) % width == 0) p += padding;
        }
        return true;
    }

    bool File::decodeGarbage(const uint8_t* src, int width, int height, std::vector<uint8_t>& outRGBA)
    {
        if (!src || width <= 0 || height <= 0) return false;
        outRGBA.assign((size_t)width * (size_t)height * 4, 0);

        const uint8_t* p = src;
        for (int i = 0; i < width * height; ++i)
        {
            uint16_t r = rd_u16(p); p += 2;
            uint16_t g = rd_u16(p); p += 2;
            uint16_t b = rd_u16(p); p += 2;
            uint16_t a = rd_u16(p); p += 2;

            outRGBA[i * 4 + 0] = (uint8_t)(r >> 8);
            outRGBA[i * 4 + 1] = (uint8_t)(g >> 8);
            outRGBA[i * 4 + 2] = (uint8_t)(b >> 8);
            outRGBA[i * 4 + 3] = (uint8_t)(a >> 8);
        }
        return true;
    }

    bool File::readFromMemory(const uint8_t* data, size_t size)
    {
        failedReading = false;
        mipData.clear();

        if (!data || size == 0)
        {
            failedReading = true;
            return false;
        }

        size_t off = 0;
        if (!header.read(data, size, off))
        {
            failedReading = true;
            return false;
        }

        if (header.version > 0)
        {
            off = 112;
            if (off > size)
            {
                failedReading = true;
                return false;
            }
        }

        int mipCount = std::max(1, header.mipCount);

        switch (header.textureType)
        {
        case TextureType::DXT1:
            {
                auto sizes = calculateDXTSizes(mipCount, header.width, header.height, 8);
                mipData.reserve((size_t)mipCount);
                for (int i = 0; i < mipCount; ++i)
                {
                    int sz = sizes[i];
                    if (sz < 0 || off + (size_t)sz > size) { failedReading = true; return false; }
                    std::vector<uint8_t> buf((size_t)sz);
                    std::memcpy(buf.data(), data + off, (size_t)sz);
                    off += (size_t)sz;
                    mipData.push_back(std::move(buf));
                }
                return true;
            }
        case TextureType::DXT3:
        case TextureType::DXT5:
            {
                auto sizes = calculateDXTSizes(mipCount, header.width, header.height, 16);
                mipData.reserve((size_t)mipCount);
                for (int i = 0; i < mipCount; ++i)
                {
                    int sz = sizes[i];
                    if (sz < 0 || off + (size_t)sz > size) { failedReading = true; return false; }
                    std::vector<uint8_t> buf((size_t)sz);
                    std::memcpy(buf.data(), data + off, (size_t)sz);
                    off += (size_t)sz;
                    mipData.push_back(std::move(buf));
                }
                return true;
            }
        case TextureType::Argb1:
        case TextureType::Argb2:
            {
                if (header.version == 0 || header.imageSizesCount == 0)
                {
                    size_t remaining = size - off;
                    std::vector<uint8_t> buf(remaining);
                    std::memcpy(buf.data(), data + off, remaining);
                    mipData.push_back(std::move(buf));
                    return true;
                }

                mipData.reserve((size_t)header.imageSizesCount);
                size_t offs = off;
                for (uint32_t i = 0; i < header.imageSizesCount; ++i)
                {
                    uint32_t sz = header.imageSizes[i];
                    if (sz == 0) { mipData.emplace_back(); continue; }
                    if (offs + (size_t)sz > size) { failedReading = true; return false; }
                    std::vector<uint8_t> buf((size_t)sz);
                    std::memcpy(buf.data(), data + offs, (size_t)sz);
                    offs += (size_t)sz;
                    mipData.push_back(std::move(buf));
                }
                return true;
            }
        case TextureType::Jpeg1:
        case TextureType::Jpeg2:
        case TextureType::Jpeg3:
            {
                mipData.reserve((size_t)header.imageSizesCount);
                for (uint32_t i = 0; i < header.imageSizesCount; ++i)
                {
                    uint32_t sz = header.imageSizes[i];
                    if (sz == 0) { mipData.emplace_back(); continue; }
                    if (off + (size_t)sz > size) { failedReading = true; return false; }
                    std::vector<uint8_t> buf((size_t)sz);
                    std::memcpy(buf.data(), data + off, (size_t)sz);
                    off += (size_t)sz;
                    mipData.push_back(std::move(buf));
                }
                return true;
            }
        case TextureType::Argb16:
        case TextureType::Grayscale:
        case TextureType::Garbage:
            {
                mipData.reserve((size_t)mipCount);
                for (int i = mipCount - 1; i >= 0; --i)
                {
                    int m = i;
                    int w = (int)(header.width / std::pow(2.0, (double)m));
                    int h = (int)(header.height / std::pow(2.0, (double)m));
                    if (w < 1) w = 1;
                    if (h < 1) h = 1;

                    size_t levelSize = 0;
                    if (header.textureType == TextureType::Argb16) {
                        levelSize = (size_t)(w * h * 2);
                    } else if (header.textureType == TextureType::Grayscale) {
                         size_t padding = (4 - (w % 4)) % 4;
                         levelSize = (size_t)((w + padding) * h);
                    } else if (header.textureType == TextureType::Garbage) {
                        levelSize = (size_t)(w * h * 8);
                    }

                    if (off + levelSize > size) { failedReading = true; return false; }
                    std::vector<uint8_t> buf(levelSize);
                    std::memcpy(buf.data(), data + off, levelSize);
                    off += levelSize;

                    mipData.insert(mipData.begin(), std::move(buf));
                }
                return true;
            }

        case TextureType::Rgb:
        case TextureType::Unknown:
        default:
            failedReading = true;
            return false;
        }
    }

    bool File::decodeLargestMipToRGBA(ImageRGBA& out) const
    {
        out = {};
        if (failedReading) return false;
        if (header.width <= 0 || header.height <= 0) return false;
        if (mipData.empty()) return false;

        auto largestMipIndex = (int)mipData.size() - 1;
        int w = header.width;
        int h = header.height;

        bool success = false;
        bool preserveAlpha = false;

        if (header.textureType == TextureType::DXT1)
        {
            out.width = w; out.height = h;
            success = decodeDXT1(mipData[largestMipIndex].data(), w, h, out.rgba);
        }
        else if (header.textureType == TextureType::DXT3)
        {
            out.width = w; out.height = h;
            success = decodeDXT3(mipData[largestMipIndex].data(), w, h, out.rgba);
            preserveAlpha = true;
        }
        else if (header.textureType == TextureType::DXT5)
        {
            out.width = w; out.height = h;
            success = decodeDXT5(mipData[largestMipIndex].data(), w, h, out.rgba);
            preserveAlpha = true;
        }
        else if (header.textureType == TextureType::Jpeg1 ||
            header.textureType == TextureType::Jpeg2 ||
            header.textureType == TextureType::Jpeg3)
        {
            int mipLevel = (int)(header.imageSizesCount - 1) - largestMipIndex;
            success = Jpeg::Decode(header, mipLevel, mipData[largestMipIndex], out.rgba, out.width, out.height);
            preserveAlpha = true;
        }
        else if (header.textureType == TextureType::Argb1 || header.textureType == TextureType::Argb2)
        {
            const auto& buf = mipData[largestMipIndex];
            if (buf.empty()) return false;
            size_t expected = (size_t)w * (size_t)h * 4;
            if (buf.size() < expected) return false;
            out.width = w; out.height = h;
            out.rgba.assign(buf.begin(), buf.begin() + expected);
            for(size_t i=0; i<out.rgba.size(); i+=4) {
                std::swap(out.rgba[i], out.rgba[i+2]);
            }
            success = true;
        }
        else if (header.textureType == TextureType::Argb16)
        {
            out.width = w; out.height = h;
            success = decodeArgb16(mipData[largestMipIndex].data(), w, h, out.rgba);
        }
        else if (header.textureType == TextureType::Grayscale)
        {
            out.width = w; out.height = h;
            success = decodeGrayscale(mipData[largestMipIndex].data(), w, h, out.rgba);
        }
        else if (header.textureType == TextureType::Garbage)
        {
            out.width = w; out.height = h;
            success = decodeGarbage(mipData[largestMipIndex].data(), w, h, out.rgba);
        }

        if (success && !preserveAlpha)
        {
            for (size_t i = 3; i < out.rgba.size(); i += 4)
            {
                out.rgba[i] = 255;
            }
        }

        return success;
    }

    bool File::decodeThumbnailMipToRGBA(ImageRGBA& out, int maxSize) const
    {
        out = {};
        if (failedReading) return false;
        if (header.width <= 0 || header.height <= 0) return false;
        if (mipData.empty()) return false;

        int mipIndex = 0;
        int w = header.width;
        int h = header.height;

        for (int i = (int)mipData.size() - 1; i >= 0; --i)
        {
            int mipW = (int)(header.width / std::pow(2.0, (double)((int)mipData.size() - 1 - i)));
            int mipH = (int)(header.height / std::pow(2.0, (double)((int)mipData.size() - 1 - i)));
            if (mipW < 1) mipW = 1;
            if (mipH < 1) mipH = 1;

            if (mipW <= maxSize && mipH <= maxSize)
            {
                mipIndex = i;
                w = mipW;
                h = mipH;
                break;
            }
            mipIndex = i;
            w = mipW;
            h = mipH;
        }

        if (mipData[mipIndex].empty()) return false;

        bool success = false;
        bool preserveAlpha = false;

        if (header.textureType == TextureType::DXT1)
        {
            out.width = w; out.height = h;
            success = decodeDXT1(mipData[mipIndex].data(), w, h, out.rgba);
        }
        else if (header.textureType == TextureType::DXT3)
        {
            out.width = w; out.height = h;
            success = decodeDXT3(mipData[mipIndex].data(), w, h, out.rgba);
            preserveAlpha = true;
        }
        else if (header.textureType == TextureType::DXT5)
        {
            out.width = w; out.height = h;
            success = decodeDXT5(mipData[mipIndex].data(), w, h, out.rgba);
            preserveAlpha = true;
        }
        else if (header.textureType == TextureType::Jpeg1 ||
            header.textureType == TextureType::Jpeg2 ||
            header.textureType == TextureType::Jpeg3)
        {
            int mipLevel = (int)(header.imageSizesCount - 1) - mipIndex;
            success = Jpeg::Decode(header, mipLevel, mipData[mipIndex], out.rgba, out.width, out.height);
            preserveAlpha = true;
        }
        else if (header.textureType == TextureType::Argb1 || header.textureType == TextureType::Argb2)
        {
            const auto& buf = mipData[mipIndex];
            if (buf.empty()) return false;
            size_t expected = (size_t)w * (size_t)h * 4;
            if (buf.size() < expected) return false;
            out.width = w; out.height = h;
            out.rgba.assign(buf.begin(), buf.begin() + expected);
            for(size_t i=0; i<out.rgba.size(); i+=4) {
                std::swap(out.rgba[i], out.rgba[i+2]);
            }
            success = true;
        }
        else if (header.textureType == TextureType::Argb16)
        {
            out.width = w; out.height = h;
            success = decodeArgb16(mipData[mipIndex].data(), w, h, out.rgba);
        }
        else if (header.textureType == TextureType::Grayscale)
        {
            out.width = w; out.height = h;
            success = decodeGrayscale(mipData[mipIndex].data(), w, h, out.rgba);
        }
        else if (header.textureType == TextureType::Garbage)
        {
            out.width = w; out.height = h;
            success = decodeGarbage(mipData[mipIndex].data(), w, h, out.rgba);
        }

        if (success && !preserveAlpha)
        {
            for (size_t i = 3; i < out.rgba.size(); i += 4)
            {
                out.rgba[i] = 255;
            }
        }

        return success;
    }

    ComPtr<ID3D11ShaderResourceView> CreateSRVFromFile(const File& tf)
    {
        ID3D11Device* device = GetDevice();
        if (!device || tf.mipData.empty()) return nullptr;

        int w = tf.header.width;
        int h = tf.header.height;

        DXGI_FORMAT compressedFmt = DXGI_FORMAT_UNKNOWN;
        switch(tf.header.textureType) {
            case TextureType::DXT1: compressedFmt = DXGI_FORMAT_BC1_UNORM; break;
            case TextureType::DXT3: compressedFmt = DXGI_FORMAT_BC2_UNORM; break;
            case TextureType::DXT5: compressedFmt = DXGI_FORMAT_BC3_UNORM; break;
            default: break;
        }

        ComPtr<ID3D11Texture2D> tex;
        ComPtr<ID3D11ShaderResourceView> srv;

        if (compressedFmt != DXGI_FORMAT_UNKNOWN)
        {
            const auto& data = tf.mipData.back();

            D3D11_TEXTURE2D_DESC td = {};
            td.Width = w;
            td.Height = h;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = compressedFmt;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_IMMUTABLE;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            int blockSize = (compressedFmt == DXGI_FORMAT_BC1_UNORM) ? 8 : 16;
            UINT rowPitch = ((w + 3) / 4) * blockSize;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = data.data();
            initData.SysMemPitch = rowPitch;

            if (FAILED(device->CreateTexture2D(&td, &initData, &tex))) return nullptr;
        }
        else
        {
            ImageRGBA img;
            if (!tf.decodeLargestMipToRGBA(img)) return nullptr;

            w = img.width;
            h = img.height;

            D3D11_TEXTURE2D_DESC td = {};
            td.Width = w;
            td.Height = h;
            td.MipLevels = 1;
            td.ArraySize = 1;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_IMMUTABLE;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = img.rgba.data();
            initData.SysMemPitch = w * 4;

            if (FAILED(device->CreateTexture2D(&td, &initData, &tex))) return nullptr;
        }

        if (FAILED(device->CreateShaderResourceView(tex.Get(), nullptr, &srv))) return nullptr;

        return srv;
    }

    bool OpenTexPreviewFromEntry(AppState& state, const ArchivePtr& arc, const FileEntryPtr& fileEntry)
    {
        if (!state.texPreview) return false;

        state.texPreview->textureSRV.Reset();
        state.texPreview->hasTexture = false;
        state.texPreview->texW = 0;
        state.texPreview->texH = 0;
        state.texPreview->open = true;
        state.texPreview->title = "Processing...";
        state.texPreview->ownsTexture = true;

        if (!arc || !fileEntry) {
            state.texPreview->title = "Error: Invalid archive or file";
            return false;
        }

        std::vector<uint8_t> bytes;
        arc->getFileData(fileEntry, bytes);

        if (bytes.empty()) {
            state.texPreview->title = "Error: Could not read file data";
            return false;
        }

        File tf;
        if (!tf.readFromMemory(bytes.data(), bytes.size())) {
            state.texPreview->title = "Error: Unsupported texture format";
            return false;
        }

        if (!GetDevice()) {
            state.texPreview->title = "Error: Graphics device not initialized";
            return false;
        }

        state.texPreview->textureSRV = CreateSRVFromFile(tf);
        state.texPreview->hasTexture = (state.texPreview->textureSRV != nullptr);
        state.texPreview->texW = tf.header.width;
        state.texPreview->texH = tf.header.height;

        if (state.texPreview->hasTexture) {
            state.texPreview->title = "Texture Preview";
        } else {
            state.texPreview->title = "Error: Failed to create texture";
        }

        return state.texPreview->hasTexture;
    }

    void OpenTexPreviewFromSRV(AppState& state, ID3D11ShaderResourceView* srv, int width, int height, const std::string& title)
    {
        if (!state.texPreview) return;

        if (state.texPreview->ownsTexture) {
            state.texPreview->textureSRV.Reset();
        }

        state.texPreview->textureSRV = srv;
        state.texPreview->texW = width;
        state.texPreview->texH = height;
        state.texPreview->hasTexture = (srv != nullptr);
        state.texPreview->ownsTexture = false;
        state.texPreview->open = true;
        state.texPreview->title = title;
    }

    void RenderTexPreviewWindow(PreviewState& ps)
    {
        if (!ps.open) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 center = viewport->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::SetNextWindowSize(ImVec2(650, 580), ImGuiCond_Appearing);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin(ps.title.c_str(), &ps.open, flags))
        {
            if (ps.hasTexture && ps.textureSRV)
            {
                float controlWidth = ImGui::CalcTextSize("Opaque Preview").x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::GetStyle().WindowPadding.x * 2.0f;
                if (controlWidth < 120.0f) controlWidth = 120.0f;

                if (ImGui::BeginTable("TexLayout", 2, ImGuiTableFlags_Resizable))
                {
                    ImGui::TableSetupColumn("Image", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, controlWidth);

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    avail.y -= ImGui::GetTextLineHeightWithSpacing();

                    float w = (float)ps.texW;
                    float h = (float)ps.texH;

                    float scale = 1.0f;
                    if (w > 0.0f && h > 0.0f)
                    {
                        float scaleX = avail.x / w;
                        float scaleY = avail.y / h;
                        scale = (scaleX < scaleY) ? scaleX : scaleY;
                    }

                    ImVec2 drawSize(w * scale, h * scale);

                    float cursorX = ImGui::GetCursorPosX() + (avail.x - drawSize.x) * 0.5f;
                    float cursorY = ImGui::GetCursorPosY() + (avail.y - drawSize.y) * 0.5f;

                    ImGui::SetCursorPos(ImVec2(cursorX, cursorY));

                    ImVec4 tint(
                        ps.showR ? 1.0f : 0.0f,
                        ps.showG ? 1.0f : 0.0f,
                        ps.showB ? 1.0f : 0.0f,
                        1.0f
                    );

                    ImGui::Image((ImTextureID)ps.textureSRV.Get(), drawSize, ImVec2(0,0), ImVec2(1,1), tint, ImVec4(0,0,0,0));

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Channels");
                    ImGui::Separator();
                    ImGui::Checkbox("Red", &ps.showR);
                    ImGui::Checkbox("Green", &ps.showG);
                    ImGui::Checkbox("Blue", &ps.showB);
                    ImGui::Checkbox("Alpha", &ps.showA);

                    ImGui::EndTable();
                }

                ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowHeight() - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().WindowPadding.y));
                ImGui::Text("Size: %dx%d", ps.texW, ps.texH);
            }
            else
            {
                ImGui::Text("No texture loaded or invalid texture ID.");
            }
        }
        ImGui::End();

        if (!ps.open)
        {
            ps.clear();
        }
    }

    static bool WriteDDS(const std::string& path, const uint8_t* rgba, int w, int h)
    {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        uint32_t magic = 0x20534444;
        f.write((char*)&magic, 4);

        uint32_t header[31] = {0};
        header[0] = 124;
        header[1] = 0x1 | 0x2 | 0x4 | 0x1000 | 0x8;
        header[2] = h;
        header[3] = w;
        header[4] = w * 4;
        header[5] = 0;
        header[6] = 1;
        header[19] = 32;
        header[20] = 0x41;
        header[21] = 0;
        header[22] = 32;
        header[23] = 0x00FF0000;
        header[24] = 0x0000FF00;
        header[25] = 0x000000FF;
        header[26] = 0xFF000000;

        header[27] = 0x1000;

        f.write((char*)header, 124);
        std::vector<uint8_t> bgra(w * h * 4);
        for (int i = 0; i < w * h; i++)
        {
            bgra[i * 4 + 0] = rgba[i * 4 + 2];
            bgra[i * 4 + 1] = rgba[i * 4 + 1];
            bgra[i * 4 + 2] = rgba[i * 4 + 0];
            bgra[i * 4 + 3] = rgba[i * 4 + 3];
        }
        f.write((char*)bgra.data(), bgra.size());

        return f.good();
    }

    static bool WriteTIFF(const std::string& path, const uint8_t* rgba, int w, int h)
    {
        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        uint32_t imageDataSize = w * h * 4;
        uint32_t ifdOffset = 8;
        uint32_t stripOffset = 8 + 2 + (12 * 14) + 4;
        uint8_t header[8] = {
            0x49, 0x49,
            0x2A, 0x00,
            0x08, 0x00, 0x00, 0x00
        };
        f.write((char*)header, 8);
        uint16_t entryCount = 14;
        f.write((char*)&entryCount, 2);

        auto writeTag = [&](uint16_t tag, uint16_t type, uint32_t count, uint32_t value) {
            f.write((char*)&tag, 2);
            f.write((char*)&type, 2);
            f.write((char*)&count, 4);
            f.write((char*)&value, 4);
        };

        writeTag(256, 3, 1, w);
        writeTag(257, 3, 1, h);
        writeTag(258, 3, 4, stripOffset + imageDataSize);
        writeTag(259, 3, 1, 1);
        writeTag(262, 3, 1, 2);
        writeTag(273, 4, 1, stripOffset);
        writeTag(277, 3, 1, 4);
        writeTag(278, 4, 1, h);
        writeTag(279, 4, 1, imageDataSize);
        writeTag(282, 5, 1, stripOffset + imageDataSize + 8);
        writeTag(283, 5, 1, stripOffset + imageDataSize + 16);
        writeTag(284, 3, 1, 1);
        writeTag(296, 3, 1, 2);
        writeTag(338, 3, 1, 1);
        uint32_t nextIFD = 0;
        f.write((char*)&nextIFD, 4);
        f.write((char*)rgba, imageDataSize);
        uint16_t bps[4] = {8, 8, 8, 8};
        f.write((char*)bps, 8);
        uint32_t xres[2] = {72, 1};
        f.write((char*)xres, 8);
        uint32_t yres[2] = {72, 1};
        f.write((char*)yres, 8);

        return f.good();
    }

    bool ExportTexture(const File& tf, const std::string& outputPath, ExportFormat format, int jpegQuality)
    {
        ImageRGBA img;
        if (!tf.decodeLargestMipToRGBA(img))
            return false;

        if (img.rgba.empty() || img.width <= 0 || img.height <= 0)
            return false;

        switch (format)
        {
        case ExportFormat::DDS:
            return WriteDDS(outputPath, img.rgba.data(), img.width, img.height);

        case ExportFormat::PNG:
            return stbi_write_png(outputPath.c_str(), img.width, img.height, 4, img.rgba.data(), img.width * 4) != 0;

        case ExportFormat::JPEG:
            return stbi_write_jpg(outputPath.c_str(), img.width, img.height, 4, img.rgba.data(), jpegQuality) != 0;

        case ExportFormat::TIFF:
            return WriteTIFF(outputPath, img.rgba.data(), img.width, img.height);

        default:
            return false;
        }
    }

    bool ExportTextureFromArchive(const ArchivePtr& arc, const FileEntryPtr& entry, const std::string& outputPath, ExportFormat format, int jpegQuality)
    {
        if (!arc || !entry) return false;

        std::vector<uint8_t> bytes;
        arc->getFileData(entry, bytes);
        if (bytes.empty()) return false;

        File tf;
        if (!tf.readFromMemory(bytes.data(), bytes.size()))
            return false;

        return ExportTexture(tf, outputPath, format, jpegQuality);
    }
}