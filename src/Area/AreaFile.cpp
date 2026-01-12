#include "AreaFile.h"
#include <glad/glad.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>

const float AreaFile::UnitSize = 2.0f;
std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

const int AreaChunkRender::DataSizes[32] =
{
    722, 16, 8450, 8450, 8450, 4, 64, 16,
    4225, 2178, 4, 578, 1, 4624, 2312, 8450,
    4096, 2312, 2312, 2312, 2312, 1, 16, 16900,
    8, 8450, 21316, 4096, 16, 8450, 8450, 2312
};

static inline std::string fourccStr(uint32 v)
{
    // interpret v as little-endian bytes for display
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

AreaFile::AreaFile(ArchivePtr archive, FileEntryPtr file) : mArchive(archive), mFile(file)
{
    if (mArchive && mFile)
    {
        mPath = mFile->getFullPath();
        try
        {
            mArchive->getFileData(mFile, mContent);
            mStream = std::make_shared<BinStream>(mContent);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error reading file content: " << e.what() << std::endl;
        }
    }
}

bool AreaFile::load()
{
    std::cout << "=== AreaFile::load() ===\n";

    if (mContent.size() < 8)
    {
        std::cout << "FAIL: file too small\n";
        return false;
    }

    struct R
    {
        const uint8* p;
        const uint8* e;

        bool can(size_t n) const { return (size_t)(e - p) >= n; }

        uint32 u32le()
        {
            if (!can(4)) return 0;
            uint32 v = (uint32)p[0] |
                ((uint32)p[1] << 8) |
                ((uint32)p[2] << 16) |
                ((uint32)p[3] << 24);
            p += 4;
            return v;
        }

        void bytes(void* out, size_t n)
        {
            if (!can(n)) return;
            memcpy(out, p, n);
            p += n;
        }

        void skip(size_t n)
        {
            if (!can(n)) { p = e; return; }
            p += n;
        }

        size_t pos(const uint8* base) const { return (size_t)(p - base); }
    };

    // IMPORTANT:
    // WildStar .area 4CCs are stored “reversed” relative to what you expect:
    // signature is typically 'aera' (0x61726561), not 'area'.
    const uint32 MAGIC_aera = 0x61726561u; // 'aera'
    const uint32 MAGIC_AREA = 0x41524541u; // 'AREA' (some files)
    const uint32 MAGIC_KNHC = 0x43484E4Bu; // 'KNHC' == "CHNK" reversed in-file

    const uint8* base = mContent.data();
    R r{ base, base + mContent.size() };

    uint32 signature = r.u32le();
    uint32 version   = r.u32le();

    std::cout << "Signature raw=0x" << std::hex << signature
        << " ('" << fourccStr(signature) << "')"
        << std::dec << "\n";
    std::cout << "Version: " << version << "\n";

    if (signature != MAGIC_aera && signature != MAGIC_AREA)
    {
        std::cout << "FAIL: invalid signature\n";
        return false;
    }

    std::vector<uint8> chnkData;

    while (r.can(8))
    {
        size_t chunkPos = r.pos(base);

        uint32 magic = r.u32le();     // DO NOT SWAP
        uint32 size  = r.u32le();     // DO NOT SWAP

        std::cout << "Outer chunk @ " << chunkPos
            << " magic=0x" << std::hex << magic
            << " ('" << fourccStr(magic) << "')"
            << std::dec
            << " size=" << size << "\n";

        if (!r.can(size))
        {
            std::cout << "FAIL: outer chunk overrun (need " << size
                << " bytes, have " << (size_t)(r.e - r.p) << ")\n";
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
        std::cout << "FAIL: missing CHNK/KNHC\n";
        return false;
    }

    // ---- Your existing CHNK parsing (kept), just with NO endian swaps ----

    mChunks.assign(256, nullptr);

    const uint8* cbase = chnkData.data();
    R cr{ cbase, cbase + chnkData.size() };

    uint32 validCount = 0;
    float totalHeight = 0.0f;
    uint32 lastIndex = 0;

    const float chunkSpan = 16.0f * UnitSize;
    const float fileSpan  = 16.0f * chunkSpan;

    const float fileBaseX = (float)mTileX * fileSpan;
    const float fileBaseY = (float)mTileY * fileSpan;

    while (cr.can(4))
    {
        uint32 cellInfo = cr.u32le();       // DO NOT SWAP

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

        // flags: LE uint32 in-file
        uint32 flags = *(const uint32*)cellData.data();

        std::vector<uint8> payload(cellData.begin() + 4, cellData.end());

        float baseX = fileBaseX + (float)(index % 16) * chunkSpan;
        float baseY = fileBaseY + (float)(index / 16) * chunkSpan;

        auto chunk = std::make_shared<AreaChunkRender>(flags, payload, baseX, baseY);
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
    return true;
}

void AreaFile::render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram)
{
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &matView[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &matProj[0][0]);

    for (auto& c : mChunks)
    {
        if (c) c->render();
    }
}

AreaChunkRender::AreaChunkRender(uint32 flags, const std::vector<uint8>& payload, float baseX, float baseY)
    : mChunkData(payload), mFlags(flags)
{
    struct R
    {
        const uint8* p;
        const uint8* e;

        bool can(size_t n) const { return (size_t)(e - p) >= n; }

        uint32 u32le()
        {
            if (!can(4)) return 0;
            uint32 v = (uint32)p[0] |
                ((uint32)p[1] << 8) |
                ((uint32)p[2] << 16) |
                ((uint32)p[3] << 24);
            p += 4;
            return v;
        }

        uint16 u16le()
        {
            if (!can(2)) return 0;
            uint16 v = (uint16)p[0] | (uint16)(p[1] << 8);
            p += 2;
            return v;
        }

        void skip(size_t n)
        {
            if (!can(n)) { p = e; return; }
            p += n;
        }
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
                uint32 v = r.u32le();   // DO NOT SWAP
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
                    uint16 hv = r.u16le();     // DO NOT SWAP
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
    std::vector<AreaVertex> vertices(17 * 33); // 17 rows of 33
    mFullVertices = mVertices;

    // copy the 17x17 grid to [0..16][0..16]
    for (uint32 i = 0; i < 17; ++i)
    {
        for (uint32 j = 0; j < 17; ++j)
        {
            vertices[i * 33 + j] = mFullVertices[(i + 1) * 19 + (j + 1)];
        }
    }

    // fill the center points (16x16) at [0..15][0..15] offset by 17
    // NOTE: your original layout expects these to sit in the same linear array.
    // Keeping your original indexing scheme:
    vertices.resize(17 * 17 + 16 * 16);

    for (uint32 i = 0; i < 16; ++i)
    {
        for (uint32 j = 0; j < 16; ++j)
        {
            uint32 idx = 17 + i * 33 + j;
            const auto& tl = mFullVertices[(i + 1) * 19 + (j + 1)];
            const auto& tr = mFullVertices[(i + 1) * 19 + (j + 2)];
            const auto& bl = mFullVertices[(i + 2) * 19 + (j + 1)];
            const auto& br = mFullVertices[(i + 2) * 19 + (j + 2)];

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
