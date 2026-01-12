#include "AreaFile.h"
#include <glad/glad.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string>
#include <zlib.h>
#include <map>
#include <memory>
#include <cstring>
#include <cwctype>
#include "../tex/tex.h"

const float AreaFile::UnitSize = 2.0f;
std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

bool AreaFile::sOriginSet = false;
int  AreaFile::sOriginTileX = 0;
int  AreaFile::sOriginTileY = 0;

const int AreaChunkRender::DataSizes[32] =
{
    722, 16, 8450, 8450, 8450, 4, 64, 16,
    4225, 2178, 4, 578, 1, 4624, 2312, 8450,
    4096, 2312, 2312, 2312, 2312, 1, 16, 16900,
    8, 8450, 21316, 4096, 16, 8450, 8450, 2312
};

static inline uint32 bswap32_u(uint32 v)
{
    return (v >> 24) |
           ((v >> 8) & 0x0000FF00u) |
           ((v << 8) & 0x00FF0000u) |
           (v << 24);
}

static inline std::string fourccStr(uint32 v)
{
    char s[5];
    s[0] = (char)(v & 0xFF);
    s[1] = (char)((v >> 8) & 0xFF);
    s[2] = (char)((v >> 16) & 0xFF);
    s[3] = (char)((v >> 24) & 0xFF);
    s[4] = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 126) s[i] = '.';
    }
    return std::string(s);
}

static inline int hexNibble(wchar_t c)
{
    if (c >= L'0' && c <= L'9') return (int)(c - L'0');
    if (c >= L'a' && c <= L'f') return 10 + (int)(c - L'a');
    if (c >= L'A' && c <= L'F') return 10 + (int)(c - L'A');
    return -1;
}

static inline bool parseHexByte(const std::wstring& s, size_t pos, int& outByte)
{
    if (pos + 2 > s.size()) return false;
    int hi = hexNibble(s[pos]);
    int lo = hexNibble(s[pos + 1]);
    if (hi < 0 || lo < 0) return false;
    outByte = (hi << 4) | lo;
    return true;
}

void AreaFile::parseTileXYFromFilename()
{
    if (mPath.empty())
        return;

    std::wstring name = mPath;

    size_t ext = name.rfind(L".area");
    if (ext == std::wstring::npos)
        return;

    if (ext < 5)
        return;

    size_t dot = ext - 5;
    if (name[dot] != L'.')
        return;

    int bx = 0, by = 0;
    if (!parseHexByte(name, dot + 1, bx)) return;
    if (!parseHexByte(name, dot + 3, by)) return;

    mTileX = bx;
    mTileY = by;
}

AreaFile::AreaFile(ArchivePtr archive, FileEntryPtr file) : mArchive(archive), mFile(file)
{
    if (mArchive && mFile)
    {
        mPath = mFile->getFullPath();
        try
        {
            mArchive->getFileData(mFile, mContent);
            mStream = std::make_shared<BinStream>(mContent);
            parseTileXYFromFilename();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error reading file content: " << e.what() << std::endl;
        }
    }
}

AreaFile::~AreaFile()
{
    if (mTextureID != 0)
    {
        glDeleteTextures(1, &mTextureID);
        mTextureID = 0;
    }
}

static std::wstring ReplaceExtension(const std::wstring& path, const std::wstring& newExt)
{
    size_t lastDot = path.find_last_of(L'.');
    if (lastDot == std::wstring::npos) return path + newExt;
    return path.substr(0, lastDot) + newExt;
}

bool AreaFile::loadTexture()
{
    if (!mFile || !mArchive) return false;

    auto parentDir = mFile->getParent();
    if (!parentDir) return false;

    std::wstring areaName = mFile->getEntryName();
    std::wstring texName = ReplaceExtension(areaName, L".tex");

    FileEntryPtr texEntry = nullptr;
    for (auto& child : parentDir->getChildren())
    {
        if (child->getEntryName() == texName && !child->isDirectory())
        {
            texEntry = std::dynamic_pointer_cast<FileEntry>(child);
            break;
        }
    }

    if (!texEntry) return false;

    std::vector<uint8_t> texBytes;
    if (!mArchive->getFileData(texEntry, texBytes) || texBytes.empty())
        return false;

    Tex::File tf;
    if (!tf.readFromMemory(texBytes.data(), texBytes.size())) return false;

    Tex::ImageRGBA img;
    if (!tf.decodeLargestMipToRGBA(img)) return false;

    glGenTextures(1, &mTextureID);
    glBindTexture(GL_TEXTURE_2D, mTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    mHasTexture = true;
    return true;
}

bool AreaFile::load()
{
    std::cout << "=== AreaFile::load() ===\n";

    if (mContent.size() < 8)
    {
        std::cout << "FAIL: stream invalid or file too small\n";
        return false;
    }

    struct R
    {
        const uint8* p;
        const uint8* e;

        bool can(size_t n) const { return (size_t)(e - p) >= n; }

        uint32 u32le()
        {
            uint32 v = (uint32)p[0] |
                       ((uint32)p[1] << 8) |
                       ((uint32)p[2] << 16) |
                       ((uint32)p[3] << 24);
            p += 4;
            return v;
        }

        uint16 u16le()
        {
            uint16 v = (uint16)p[0] | (uint16)(p[1] << 8);
            p += 2;
            return v;
        }

        void bytes(void* out, size_t n)
        {
            memcpy(out, p, n);
            p += n;
        }

        void skip(size_t n) { p += n; }
        size_t pos(const uint8* base) const { return (size_t)(p - base); }
    };

    const uint32 MAGIC_aera = 0x61726561u;
    const uint32 MAGIC_AERA = 0x41524541u;
    const uint32 MAGIC_KNHC = 0x43484e4Bu;

    const uint8* base = mContent.data();
    R r{ base, base + mContent.size() };

    uint32 sig_raw = r.u32le();
    uint32 ver_raw = r.u32le();

    std::cout << "Signature: 0x" << std::hex << sig_raw
              << " ('" << fourccStr(sig_raw) << "') swapped=0x" << bswap32_u(sig_raw)
              << " ('" << fourccStr(bswap32_u(sig_raw)) << "')"
              << std::dec << "\n";
    std::cout << "Version: " << ver_raw << "\n";

    if (sig_raw != MAGIC_aera && sig_raw != MAGIC_AERA)
    {
        std::cout << "FAIL: invalid signature\n";
        return false;
    }

    std::vector<uint8> chnkData;

    while (r.can(8))
    {
        size_t chunkPos = r.pos(base);

        uint32 magic = r.u32le();
        uint32 size  = r.u32le();

        std::cout << "Outer chunk @ " << chunkPos
                  << " magic=0x" << std::hex << magic
                  << " ('" << fourccStr(magic) << "') swapped=0x" << bswap32_u(magic)
                  << " ('" << fourccStr(bswap32_u(magic)) << "')"
                  << std::dec
                  << " size=" << size << "\n";

        if (!r.can(size))
        {
            std::cout << "FAIL: chunk size overrun\n";
            return false;
        }

        if (magic == MAGIC_KNHC)
        {
            chnkData.resize(size);
            r.bytes(chnkData.data(), size);
            break;
        }

        r.skip(size);
    }

    if (chnkData.empty())
    {
        std::cout << "FAIL: missing CHNK\n";
        return false;
    }

    if (!sOriginSet)
    {
        sOriginSet = true;
        sOriginTileX = mTileX;
        sOriginTileY = mTileY;
    }

    mChunks.assign(256, nullptr);

    const uint8* cbase = chnkData.data();
    R cr{ cbase, cbase + chnkData.size() };

    uint32 validCount = 0;
    float totalHeight = 0.0f;
    uint32 lastIndex = 0;

    const float chunkSpan = 16.0f * UnitSize;
    const float fileSpan  = 16.0f * chunkSpan;

    const float fileBaseX = (float)(mTileX - sOriginTileX) * fileSpan;
    const float fileBaseY = (float)(mTileY - sOriginTileY) * fileSpan;

    while (cr.can(4))
    {
        uint32 cellInfo = cr.u32le();

        uint32 idxDelta = (cellInfo >> 24) & 0xFF;
        uint32 size     = (cellInfo & 0x00FFFFFF);

        uint32 index = lastIndex + idxDelta;
        lastIndex = index + 1;

        if (index >= 256)
            break;

        if (!cr.can(size))
            break;

        if (size < 4)
        {
            cr.skip(size);
            continue;
        }

        std::vector<uint8> cellData(size);
        cr.bytes(cellData.data(), size);

        uint32 flags = *(const uint32*)cellData.data();
        std::vector<uint8> payload(cellData.begin() + 4, cellData.end());

        float baseX = fileBaseX + (float)(index % 16) * chunkSpan;
        float baseY = fileBaseY + (float)(index / 16) * chunkSpan;

        auto chunk = std::make_shared<AreaChunkRender>(flags, payload, baseX, baseY, false);
        mChunks[index] = chunk;

        if (chunk && chunk->hasHeightmap())
        {
            totalHeight += chunk->getAverageHeight();
            validCount++;
            mMaxHeight = std::max(mMaxHeight, chunk->getMaxHeight());
        }
    }

    if (validCount == 0)
    {
        std::cout << "FAIL: no valid height chunks\n";
        return false;
    }

    mAverageHeight = totalHeight / (float)validCount;

    loadTexture();
    return true;
}

void AreaFile::render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram)
{
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &matView[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &matProj[0][0]);

    if (mHasTexture && mTextureID != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTextureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "texSampler"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), 1);
    }
    else
    {
        glUniform1i(glGetUniformLocation(shaderProgram, "hasTexture"), 0);
    }

    for (auto& c : mChunks)
    {
        if (c) c->render();
    }
}

AreaChunkRender::AreaChunkRender(uint32 flags, const std::vector<uint8>& payload, float baseX, float baseY, bool swapped)
    : mChunkData(payload), mFlags(flags)
{
    (void)swapped;

    struct R
    {
        const uint8* p;
        const uint8* e;

        bool can(size_t n) const { return (size_t)(e - p) >= n; }

        uint32 u32le()
        {
            uint32 v = (uint32)p[0] |
                       ((uint32)p[1] << 8) |
                       ((uint32)p[2] << 16) |
                       ((uint32)p[3] << 24);
            p += 4;
            return v;
        }

        uint16 u16le()
        {
            uint16 v = (uint16)p[0] | (uint16)(p[1] << 8);
            p += 2;
            return v;
        }

        void skip(size_t n) { p += n; }
    };

    R r{ mChunkData.data(), mChunkData.data() + mChunkData.size() };

    bool hasHeight = false;

    for (int i = 0; i < 32; ++i)
    {
        uint32 bit = (1u << i);
        if ((mFlags & bit) == 0)
            continue;

        if (!r.can((size_t)DataSizes[i]))
            return;

        if (i == 28)
        {
            for (int j = 0; j < 4; ++j)
            {
                uint32 v = r.u32le();
                mZoneIds[j] = v;
            }
            continue;
        }

        if (i == 0)
        {
            hasHeight = true;

            float totalH = 0.0f;
            mVertices.resize(19 * 19);

            for (int yy = -1; yy < 18; ++yy)
            {
                for (int xx = -1; xx < 18; ++xx)
                {
                    uint16 hv = r.u16le();
                    uint16 h  = (uint16)(hv & 0x7FFF);

                    AreaVertex v{};
                    v.x = baseX + (float)xx * AreaFile::UnitSize;
                    v.z = baseY + (float)yy * AreaFile::UnitSize;
                    v.y = -2048.0f + (float)h / 8.0f;
                    v.u = (float)xx / 15.0f;
                    v.v = (float)yy / 15.0f;

                    if (v.y > mMaxHeight) mMaxHeight = v.y;
                    totalH += v.y;

                    mVertices[(yy + 1) * 19 + (xx + 1)] = v;
                }
            }

            mAverageHeight = totalH / (19.0f * 19.0f);
            continue;
        }

        r.skip((size_t)DataSizes[i]);
    }

    if (!hasHeight)
        return;

    if (indices.empty())
    {
        indices.resize(16 * 16 * 12);
        for (uint32 i = 0; i < 16; ++i)
        {
            for (uint32 j = 0; j < 16; ++j)
            {
                uint32 tribase = (i * 16 + j) * 12;
                uint32 ibase = i * 33 + j;

                indices[tribase]     = ibase;
                indices[tribase + 1] = ibase + 1;
                indices[tribase + 2] = ibase + 17;

                indices[tribase + 3] = ibase + 1;
                indices[tribase + 4] = ibase + 34;
                indices[tribase + 5] = ibase + 17;

                indices[tribase + 6] = ibase + 34;
                indices[tribase + 7] = ibase + 33;
                indices[tribase + 8] = ibase + 17;

                indices[tribase + 9]  = ibase + 33;
                indices[tribase + 10] = ibase;
                indices[tribase + 11] = ibase + 17;
            }
        }
    }

    extendBuffer();
    calcNormals();
    calcTangentBitangent();

    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(AreaVertex), mVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, nx));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, u));

    glBindVertexArray(0);
    mIndexCount = (int)indices.size();
}

AreaChunkRender::~AreaChunkRender()
{
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
    if (mEBO) glDeleteBuffers(1, &mEBO);
}

void AreaChunkRender::render()
{
    if (!mVAO) return;
    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AreaChunkRender::extendBuffer()
{
    std::vector<AreaVertex> vertices(17 * 17 + 16 * 16);
    mFullVertices = mVertices;

    for (uint32 i = 0; i < 17; ++i)
    {
        for (uint32 j = 0; j < 17; ++j)
        {
            vertices[i * 33 + j] = mVertices[(i + 1) * 19 + (j + 1)];
        }
    }

    for (uint32 i = 0; i < 16; ++i)
    {
        for (uint32 j = 0; j < 16; ++j)
        {
            uint32 idx = 17 + i * 33 + j;
            const auto& tl = mVertices[(i + 1) * 19 + (j + 1)];
            const auto& tr = mVertices[(i + 1) * 19 + (j + 2)];
            const auto& bl = mVertices[(i + 2) * 19 + (j + 1)];
            const auto& br = mVertices[(i + 2) * 19 + (j + 2)];

            AreaVertex v{};
            v.x = (tl.x + tr.x + bl.x + br.x) * 0.25f;
            v.y = (tl.y + tr.y + bl.y + br.y) * 0.25f;
            v.z = (tl.z + tr.z + bl.z + br.z) * 0.25f;
            v.u = (tl.u + tr.u) * 0.5f;
            v.v = (tl.v + bl.v) * 0.5f;
            vertices[idx] = v;
        }
    }

    mVertices = std::move(vertices);
}

void AreaChunkRender::calcNormals()
{
    for (auto& v : mVertices) { v.nx = 0.0f; v.ny = 0.0f; v.nz = 0.0f; }

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        uint32 i0 = indices[i];
        uint32 i1 = indices[i + 1];
        uint32 i2 = indices[i + 2];
        if (i0 >= mVertices.size() || i1 >= mVertices.size() || i2 >= mVertices.size()) continue;

        const AreaVertex& v0 = mVertices[i0];
        const AreaVertex& v1 = mVertices[i1];
        const AreaVertex& v2 = mVertices[i2];

        float ax = v1.x - v0.x;
        float ay = v1.y - v0.y;
        float az = v1.z - v0.z;

        float bx = v2.x - v0.x;
        float by = v2.y - v0.y;
        float bz = v2.z - v0.z;

        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;

        mVertices[i0].nx += nx; mVertices[i0].ny += ny; mVertices[i0].nz += nz;
        mVertices[i1].nx += nx; mVertices[i1].ny += ny; mVertices[i1].nz += nz;
        mVertices[i2].nx += nx; mVertices[i2].ny += ny; mVertices[i2].nz += nz;
    }

    for (auto& v : mVertices)
    {
        float len = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
        if (len > 1e-8f) { v.nx /= len; v.ny /= len; v.nz /= len; }
        else { v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f; }
    }
}

void AreaChunkRender::calcTangentBitangent()
{
    std::vector<glm::vec3> tan1(mVertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        uint32 i0 = indices[i];
        uint32 i1 = indices[i + 1];
        uint32 i2 = indices[i + 2];
        if (i0 >= mVertices.size() || i1 >= mVertices.size() || i2 >= mVertices.size()) continue;

        const AreaVertex& v0 = mVertices[i0];
        const AreaVertex& v1 = mVertices[i1];
        const AreaVertex& v2 = mVertices[i2];

        glm::vec3 p0(v0.x, v0.y, v0.z);
        glm::vec3 p1(v1.x, v1.y, v1.z);
        glm::vec3 p2(v2.x, v2.y, v2.z);

        glm::vec2 w0(v0.u, v0.v);
        glm::vec2 w1(v1.u, v1.v);
        glm::vec2 w2(v2.u, v2.v);

        glm::vec3 e1 = p1 - p0;
        glm::vec3 e2 = p2 - p0;
        glm::vec2 d1 = w1 - w0;
        glm::vec2 d2 = w2 - w0;

        float r = d1.x * d2.y - d1.y * d2.x;
        if (std::fabs(r) < 1e-8f) continue;
        r = 1.0f / r;

        glm::vec3 sdir = (e1 * d2.y - e2 * d1.y) * r;
        tan1[i0] += sdir; tan1[i1] += sdir; tan1[i2] += sdir;
    }

    for (size_t i = 0; i < mVertices.size(); ++i)
    {
        glm::vec3 n(mVertices[i].nx, mVertices[i].ny, mVertices[i].nz);
        glm::vec3 t = tan1[i];

        if (glm::length(t) < 1e-8f)
        {
            mVertices[i].tanx = 1.0f;
            mVertices[i].tany = 0.0f;
            mVertices[i].tanz = 0.0f;
            mVertices[i].tanw = 1.0f;
            continue;
        }

        t = glm::normalize(t - n * glm::dot(n, t));

        mVertices[i].tanx = t.x;
        mVertices[i].tany = t.y;
        mVertices[i].tanz = t.z;
        mVertices[i].tanw = 1.0f;
    }
}

void AreaChunkRender::geometryInit(uint32 program)
{
    uniforms.colorTexture = glGetUniformLocation(program, "colorTexture");
    uniforms.alphaTexture = glGetUniformLocation(program, "alphaTexture");
    uniforms.hasColorMap  = glGetUniformLocation(program, "hasColorMap");
    uniforms.texScale     = glGetUniformLocation(program, "texScale");
    uniforms.camPosition  = glGetUniformLocation(program, "camPosition");
    for (int i = 0; i < 4; ++i)
    {
        std::string t = "textures[" + std::to_string(i) + "]";
        std::string n = "normalTextures[" + std::to_string(i) + "]";
        uniforms.textures[i]       = glGetUniformLocation(program, t.c_str());
        uniforms.normalTextures[i] = glGetUniformLocation(program, n.c_str());
    }
}