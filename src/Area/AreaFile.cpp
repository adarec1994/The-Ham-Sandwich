#include "AreaFile.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string>
#include <zlib.h>
#include <map>
#include <memory>
#include <cstring>
#include <cwctype>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include "../tex/tex.h"

const float CELL_SIZE = 32.0f;
const float GRID_SIZE = 512.0f;
const float VERTEX_STEP = 2.0f;
const int WORLD_GRID_ORIGIN = 64;

static DataTablePtr gWorldLayer = nullptr;

std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

const int AreaChunkRender::DataSizes[32] =
{
    722, 16, 8450, 8450, 8450, 4, 64, 16,
    4225, 2178, 4, 578, 1, 4624, 2312, 8450,
    4096, 2312, 2312, 2312, 2312, 1, 16, 16900,
    8, 8450, 21316, 4096, 16, 8450, 8450, 2312
};

const AreaChunkRender::Uniforms& AreaChunkRender::getUniforms()
{
    return uniforms;
}

static inline int hexNibble(wchar_t c) {
    if (c >= L'0' && c <= L'9') return (int)(c - L'0');
    if (c >= L'a' && c <= L'f') return 10 + (int)(c - L'a');
    if (c >= L'A' && c <= L'F') return 10 + (int)(c - L'A');
    return -1;
}

static inline bool parseHexByte(const std::wstring& s, size_t pos, int& outByte) {
    if (pos + 2 > s.size()) return false;
    int hi = hexNibble(s[pos]); int lo = hexNibble(s[pos + 1]);
    if (hi < 0 || lo < 0) return false;
    outByte = (hi << 4) | lo;
    return true;
}

static std::wstring ReplaceExtension(const std::wstring& path, const std::wstring& newExt) {
    size_t lastDot = path.find_last_of(L'.');
    if (lastDot == std::wstring::npos) return path + newExt;
    return path.substr(0, lastDot) + newExt;
}

static GLuint LoadTextureFromArchive(ArchivePtr archive, const std::wstring& fullPath)
{
    if (!archive || fullPath.empty()) return 0;

    auto genericEntry = archive->getByPath(fullPath);
    if (!genericEntry) return 0;

    auto entry = std::dynamic_pointer_cast<FileEntry>(genericEntry);
    if (!entry) return 0;

    std::vector<uint8_t> texBytes;
    archive->getFileData(entry, texBytes);
    if (texBytes.empty()) return 0;

    Tex::File tf;
    if (!tf.readFromMemory(texBytes.data(), texBytes.size())) return 0;

    Tex::ImageRGBA img;
    if (!tf.decodeLargestMipToRGBA(img)) return 0;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return texID;
}

void AreaFile::parseTileXYFromFilename()
{
    if (mPath.empty()) return;
    std::wstring name = mPath;
    size_t ext = name.rfind(L".area");
    if (ext == std::wstring::npos || ext < 5) return;
    size_t dot = ext - 5;
    if (name[dot] != L'.') return;
    int bx = 0, by = 0;
    if (!parseHexByte(name, dot + 1, bx)) return;
    if (!parseHexByte(name, dot + 3, by)) return;
    mTileX = bx; mTileY = by;
}

AreaFile::AreaFile(ArchivePtr archive, FileEntryPtr file)
    : mArchive(archive)
    , mFile(file)
    , mMinBounds(std::numeric_limits<float>::max())
    , mMaxBounds(std::numeric_limits<float>::lowest())
{
    mBaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    if (mArchive && mFile) {
        mPath = mFile->getFullPath();
        try {
            mArchive->getFileData(mFile, mContent);
            mStream = std::make_shared<BinStream>(mContent);
            parseTileXYFromFilename();
        } catch (...) {}
    }
}

AreaFile::~AreaFile() {
    if (mTextureID != 0) glDeleteTextures(1, &mTextureID);
}

bool AreaFile::loadTexture() {
    if (!mFile || !mArchive) return false;
    std::wstring areaName = mFile->getEntryName();
    std::wstring texName = ReplaceExtension(areaName, L".tex");
    auto parent = mFile->getParent();
    if(parent) {
        for(auto& child : parent->getChildren()) {
             if(child->getEntryName() == texName) {
                 mTextureID = LoadTextureFromArchive(mArchive, child->getFullPath());
                 if(mTextureID != 0) { mHasTexture = true; return true; }
             }
        }
    }
    return false;
}

bool AreaFile::load()
{
    std::cout << "=== AreaFile::load() ===\n";
    if (mContent.size() < 8) return false;

    if (gWorldLayer == nullptr && mArchive) {
        auto genericEntry = mArchive->getByPath(L"DB\\WorldLayer.tbl");
        if (genericEntry) {
            auto fileEntry = std::dynamic_pointer_cast<FileEntry>(genericEntry);
            if (fileEntry) {
                gWorldLayer = std::make_shared<DataTable>(fileEntry, mArchive);
                if (!gWorldLayer->initialLoadIDs()) gWorldLayer = nullptr;
                else std::cout << "Loaded WorldLayer.tbl successfully.\n";
            }
        }
    }

    struct R {
        const uint8* p; const uint8* e;
        bool can(size_t n) const { return (size_t)(e - p) >= n; }
        uint32 u32le() { uint32 v = (uint32)p[0] | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) | ((uint32)p[3] << 24); p += 4; return v; }
        void bytes(void* out, size_t n) { memcpy(out, p, n); p += n; }
        void skip(size_t n) { p += n; }
    };

    const uint8* base = mContent.data();
    R r{ base, base + mContent.size() };
    uint32 sig = r.u32le(); uint32 ver = r.u32le();

    if (sig != 0x61726561u && sig != 0x41524541u) return false;

    std::vector<uint8> chnkData;
    while (r.can(8)) {
        uint32 magic = r.u32le(); uint32 size = r.u32le();
        if (!r.can(size)) break;
        if (magic == 0x43484e4Bu) {
            chnkData.resize(size);
            r.bytes(chnkData.data(), size);
        } else r.skip(size);
    }

    if (chnkData.empty()) {
        mMinBounds = glm::vec3(-100,0,-100); mMaxBounds = glm::vec3(100,100,100);
        return true;
    }

    mChunks.assign(256, nullptr);
    const uint8* cbase = chnkData.data();
    R cr{ cbase, cbase + chnkData.size() };

    uint32 validCount = 0; float totalH = 0.0f; uint32 lastIndex = 0;

    // Position 0,0 is at grid 64,64
    float fileBaseX = (float)(mTileX - WORLD_GRID_ORIGIN) * GRID_SIZE;
    float fileBaseY = (float)(mTileY - WORLD_GRID_ORIGIN) * GRID_SIZE;

    while (cr.can(4)) {
        uint32 cellInfo = cr.u32le();
        uint32 idxDelta = (cellInfo >> 24) & 0xFF;
        uint32 size = (cellInfo & 0x00FFFFFF);
        uint32 index = lastIndex + idxDelta;
        lastIndex = index + 1;
        if (index >= 256) break;
        if (!cr.can(size)) break;
        if (size < 4) { cr.skip(size); continue; }

        std::vector<uint8> cellData(size);
        cr.bytes(cellData.data(), size);
        uint32 flags = *(const uint32*)cellData.data();
        std::vector<uint8> payload(cellData.begin() + 4, cellData.end());

        // Inner cells are 16x16, each 32 units wide
        float cellOffsetX = (float)(index % 16) * CELL_SIZE;
        float cellOffsetY = (float)(index / 16) * CELL_SIZE;

        float finalBaseX = fileBaseX + cellOffsetX;
        float finalBaseY = fileBaseY + cellOffsetY;

        auto chunk = std::make_shared<AreaChunkRender>(flags, payload, finalBaseX, finalBaseY, mArchive);
        mChunks[index] = chunk;

        if (chunk && chunk->hasHeightmap()) {
            totalH += chunk->getAverageHeight();
            validCount++;
            mMaxHeight = std::max(mMaxHeight, chunk->getMaxHeight());
            mMinBounds = glm::min(mMinBounds, chunk->getMinBounds());
            mMaxBounds = glm::max(mMaxBounds, chunk->getMaxBounds());
        }
    }

    if (validCount > 0) mAverageHeight = totalH / (float)validCount;
    else {
        mMinBounds = glm::vec3(fileBaseX, 0, fileBaseY);
        mMaxBounds = glm::vec3(fileBaseX+50, 50, fileBaseY+50);
    }
    loadTexture();
    return true;
}

AreaChunkRender::AreaChunkRender(uint32 flags, const std::vector<uint8>& payload, float baseX, float baseY, ArchivePtr archive)
    : mChunkData(payload)
    , mFlags(flags)
    , mMinBounds(std::numeric_limits<float>::max())
    , mMaxBounds(std::numeric_limits<float>::lowest())
    , mSplatTexture(0)
    , mColorMapTexture(0)
{
#pragma pack(push, 1)
    struct HeightMap { uint16 data[19][19]; };
    struct BlendData { uint16 data[65][65]; };
#pragma pack(pop)

    size_t currentOffset = 0;

    for (int i = 0; i < 32; ++i)
    {
        if (mFlags & (1 << i))
        {
            size_t size = DataSizes[i];

            if (currentOffset + size > mChunkData.size()) {
                std::cout << "Chunk data overrun at flag index " << i << "\n";
                break;
            }

            if (i == 0)
            {
                HeightMap& hm = *(HeightMap*)(mChunkData.data() + currentOffset);
                float totalH = 0.0f;
                mVertices.resize(19 * 19);

                // Grid is 19x19 vertices
                for (int yy = -1; yy < 18; ++yy) {
                    for (int xx = -1; xx < 18; ++xx) {
                        int hY = std::clamp(yy + 1, 0, 18);
                        int hX = std::clamp(xx + 1, 0, 18);

                        // Mask holes
                        uint16 h = hm.data[hY][hX] & 0x7FFF;

                        AreaVertex v{};
                        // Stride 2.0 per vertex
                        v.x = baseX + (float)xx * VERTEX_STEP;
                        v.z = baseY + (float)yy * VERTEX_STEP;

                        // Normalization: (h / 8) - 2048
                        v.y = ((float)h / 8.0f) - 2048.0f;

                        v.u = (float)xx / 16.0f;
                        v.v = (float)yy / 16.0f;

                        if (v.y > mMaxHeight) mMaxHeight = v.y;
                        totalH += v.y;

                        glm::vec3 pos(v.x, v.y, v.z);
                        mMinBounds = glm::min(mMinBounds, pos);
                        mMaxBounds = glm::max(mMaxBounds, pos);
                        mVertices[(yy + 1) * 19 + (xx + 1)] = v;
                    }
                }
                mAverageHeight = totalH / (19.0f * 19.0f);
            }
            else if (i == 1)
            {
                uint32* texIds = (uint32*)(mChunkData.data() + currentOffset);

                if (archive && gWorldLayer) {
                    WorldLayerEntry layer;
                    for(int t=0; t<4; ++t) {
                        uint32 tid = texIds[t];
                        if(tid != 0 && gWorldLayer->getRowById(tid, layer)) {
                            if(layer.colorMapPath && wcslen(layer.colorMapPath) > 0) {
                                GLuint glID = LoadTextureFromArchive(archive, layer.colorMapPath);
                                mLayerTextures.push_back(glID);

                                float scale = 1.0f;
                                if (layer.metersPerTexture > 0.1f)
                                    scale = 1.0f / (layer.metersPerTexture / 32.0f);
                                ((float*)&mTexScales)[t] = scale;
                            } else {
                                mLayerTextures.push_back(0);
                            }
                        } else {
                            mLayerTextures.push_back(0);
                        }
                    }
                }
            }
            else if (i == 2)
            {
                BlendData& blend = *(BlendData*)(mChunkData.data() + currentOffset);
                mBlendValues.resize(65 * 65);
                std::fill(mBlendValues.begin(), mBlendValues.end(), 0);

                for (int t = 0; t < 4; ++t) {
                    for (uint32 j = 0; j < 65 * 65; ++j) {
                        uint16 val = blend.data[j / 65][j % 65];
                        uint32 value = (val & (0xF << (t * 4))) >> (t * 4);
                        uint8 weight = (uint8)((value / 15.0f) * 255.0f);
                        mBlendValues[j] |= (weight << (8 * t));
                    }
                }

                glGenTextures(1, &mSplatTexture);
                glBindTexture(GL_TEXTURE_2D, mSplatTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 65, 65, 0, GL_RGBA, GL_UNSIGNED_BYTE, mBlendValues.data());
            }
            currentOffset += size;
        }
    }

    if (mVertices.empty()) return;

    if (indices.empty()) {
        indices.resize(16 * 16 * 12);
        for (uint32 i = 0; i < 16; ++i) {
            for (uint32 j = 0; j < 16; ++j) {
                uint32 tribase = (i * 16 + j) * 12;
                uint32 ibase = i * 33 + j;
                indices[tribase] = ibase; indices[tribase + 1] = ibase + 1; indices[tribase + 2] = ibase + 17;
                indices[tribase + 3] = ibase + 1; indices[tribase + 4] = ibase + 34; indices[tribase + 5] = ibase + 17;
                indices[tribase + 6] = ibase + 34; indices[tribase + 7] = ibase + 33; indices[tribase + 8] = ibase + 17;
                indices[tribase + 9] = ibase + 33; indices[tribase + 10] = ibase; indices[tribase + 11] = ibase + 17;
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
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, nx));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, u));
    glBindVertexArray(0);
    mIndexCount = (int)indices.size();
}

AreaChunkRender::~AreaChunkRender() {
    if (mSplatTexture != 0) glDeleteTextures(1, &mSplatTexture);
    if (mColorMapTexture != 0) glDeleteTextures(1, &mColorMapTexture);
    if (!mLayerTextures.empty()) glDeleteTextures((GLsizei)mLayerTextures.size(), mLayerTextures.data());
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
    if (mEBO) glDeleteBuffers(1, &mEBO);
}

static GLuint gFallbackWhite = 0;
static GLuint gFallbackFlatNormal = 0;
static uint32 gLastTerrainProgram = 0;

static void EnsureFallbackTextures() {
    if (gFallbackWhite == 0) {
        uint8_t px[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &gFallbackWhite);
        glBindTexture(GL_TEXTURE_2D, gFallbackWhite);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
    if (gFallbackFlatNormal == 0) {
        uint8_t px[4] = { 128, 128, 255, 255 };
        glGenTextures(1, &gFallbackFlatNormal);
        glBindTexture(GL_TEXTURE_2D, gFallbackFlatNormal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
}

void AreaFile::render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram, AreaChunkRenderPtr selectedChunk)
{
    glUseProgram(shaderProgram);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &matView[0][0]);
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &matProj[0][0]);

    EnsureFallbackTextures();
    if (gLastTerrainProgram != shaderProgram) {
        AreaChunkRender::geometryInit(shaderProgram);
        gLastTerrainProgram = shaderProgram;
    }

    const auto& u = AreaChunkRender::getUniforms();
    if ((GLint)u.camPosition != -1) glUniform3f((GLint)u.camPosition, 0.0f, 0.0f, 0.0f);

    glm::vec4 selectionColor(0.0f, 1.0f, 0.0f, 1.0f);

    glm::mat4 globalModel(1.0f);
    globalModel = glm::rotate(globalModel, glm::radians(mGlobalRotation), glm::vec3(0.0f, 1.0f, 0.0f));

    bool isAreaSelected = false;
    if (selectedChunk) {
        for (const auto& c : mChunks) if (c == selectedChunk) { isAreaSelected = true; break; }
    }

    if (u.model != -1) glUniformMatrix4fv(u.model, 1, GL_FALSE, &globalModel[0][0]);

    if (u.highlightColor != -1) {
        if (isAreaSelected) glUniform4fv(u.highlightColor, 1, &selectionColor[0]);
        else glUniform4fv(u.highlightColor, 1, &mBaseColor[0]);
    }

    for (auto& c : mChunks) {
        if (c) c->render();
    }
}

void AreaChunkRender::render() {
    if (!mVAO) return;
    const auto& u = getUniforms();

    if (mSplatTexture != 0) {
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, mSplatTexture);
        if((GLint)u.alphaTexture != -1) glUniform1i((GLint)u.alphaTexture, 8);
    } else {
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, gFallbackWhite);
        if((GLint)u.alphaTexture != -1) glUniform1i((GLint)u.alphaTexture, 8);
    }

    for(size_t i=0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + (GLenum)i);
        if(i < mLayerTextures.size() && mLayerTextures[i] != 0) {
            glBindTexture(GL_TEXTURE_2D, mLayerTextures[i]);
        } else {
            glBindTexture(GL_TEXTURE_2D, gFallbackWhite);
        }
        if((GLint)u.textures[i] != -1) glUniform1i((GLint)u.textures[i], (GLint)i);
    }

    if ((GLint)u.texScale != -1) glUniform4fv((GLint)u.texScale, 1, &mTexScales[0]);

    for(int i=0; i<4; ++i) {
        if ((GLint)u.normalTextures[i] != -1) {
            glActiveTexture(GL_TEXTURE4 + i);
            glBindTexture(GL_TEXTURE_2D, gFallbackFlatNormal);
            glUniform1i((GLint)u.normalTextures[i], 4 + i);
        }
    }

    if ((GLint)u.hasColorMap != -1) glUniform1i((GLint)u.hasColorMap, 0);

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AreaChunkRender::extendBuffer() {
    std::vector<AreaVertex> vertices(17 * 17 + 16 * 16);
    mFullVertices = mVertices;
    for (uint32 i = 0; i < 17; ++i) {
        for (uint32 j = 0; j < 17; ++j) {
            vertices[i * 33 + j] = mVertices[(i + 1) * 19 + (j + 1)];
        }
    }
    for (uint32 i = 0; i < 16; ++i) {
        for (uint32 j = 0; j < 16; ++j) {
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

void AreaChunkRender::calcNormals() {
    for (auto& v : mVertices) { v.nx = 0.0f; v.ny = 0.0f; v.nz = 0.0f; }
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32 i0 = indices[i];
        uint32 i1 = indices[i + 1];
        uint32 i2 = indices[i + 2];
        if (i0 >= mVertices.size() || i1 >= mVertices.size() || i2 >= mVertices.size()) continue;
        const AreaVertex& v0 = mVertices[i0];
        const AreaVertex& v1 = mVertices[i1];
        const AreaVertex& v2 = mVertices[i2];
        float ax = v1.x - v0.x; float ay = v1.y - v0.y; float az = v1.z - v0.z;
        float bx = v2.x - v0.x; float by = v2.y - v0.y; float bz = v2.z - v0.z;
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;
        mVertices[i0].nx += nx; mVertices[i0].ny += ny; mVertices[i0].nz += nz;
        mVertices[i1].nx += nx; mVertices[i1].ny += ny; mVertices[i1].nz += nz;
        mVertices[i2].nx += nx; mVertices[i2].ny += ny; mVertices[i2].nz += nz;
    }
    for (auto& v : mVertices) {
        float len = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
        if (len > 1e-8f) { v.nx /= len; v.ny /= len; v.nz /= len; }
        else { v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f; }
    }
}

void AreaChunkRender::calcTangentBitangent() {
    std::vector<glm::vec3> tan1(mVertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
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
    for (size_t i = 0; i < mVertices.size(); ++i) {
        glm::vec3 n(mVertices[i].nx, mVertices[i].ny, mVertices[i].nz);
        glm::vec3 t = tan1[i];
        if (glm::length(t) < 1e-8f) {
            mVertices[i].tanx = 1.0f; mVertices[i].tany = 0.0f; mVertices[i].tanz = 0.0f; mVertices[i].tanw = 1.0f;
            continue;
        }
        t = glm::normalize(t - n * glm::dot(n, t));
        mVertices[i].tanx = t.x; mVertices[i].tany = t.y; mVertices[i].tanz = t.z; mVertices[i].tanw = 1.0f;
    }
}

void AreaChunkRender::geometryInit(uint32 program) {
    uniforms.colorTexture = glGetUniformLocation(program, "colorTexture");
    uniforms.alphaTexture = glGetUniformLocation(program, "alphaTexture");
    uniforms.hasColorMap  = glGetUniformLocation(program, "hasColorMap");
    uniforms.texScale     = glGetUniformLocation(program, "texScale");
    uniforms.camPosition  = glGetUniformLocation(program, "camPosition");
    uniforms.model        = glGetUniformLocation(program, "model");
    uniforms.highlightColor = glGetUniformLocation(program, "highlightColor");
    for (int i = 0; i < 4; ++i) {
        std::string t = "textures[" + std::to_string(i) + "]";
        std::string n = "normalTextures[" + std::to_string(i) + "]";
        uniforms.textures[i]       = glGetUniformLocation(program, t.c_str());
        uniforms.normalTextures[i] = glGetUniformLocation(program, n.c_str());
    }
}