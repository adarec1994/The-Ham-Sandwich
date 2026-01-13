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

// Constants matching the original WildStar terrain format
const float AreaFile::UnitSize = 2.0f;  // Distance between vertices
const float AreaFile::GRID_SIZE = 512.0f;  // Size of each area tile in world units
static DataTablePtr gWorldLayer = nullptr;

std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

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
    calculateWorldOffset();
}

void AreaFile::calculateWorldOffset()
{
    // World grid: tile (64,64) is at origin (0,0)
    // Each tile is 512 units
    mWorldOffset.x = (float)(mTileX - WORLD_GRID_ORIGIN) * GRID_SIZE;
    mWorldOffset.y = 0.0f;
    mWorldOffset.z = (float)(mTileY - WORLD_GRID_ORIGIN) * GRID_SIZE;
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
    if (mContent.size() < 8) return false;

    // Load WorldLayer table for texture lookups
    if (gWorldLayer == nullptr && mArchive) {
        auto genericEntry = mArchive->getByPath(L"DB\\WorldLayer.tbl");
        if (genericEntry) {
            auto fileEntry = std::dynamic_pointer_cast<FileEntry>(genericEntry);
            if (fileEntry) {
                gWorldLayer = std::make_shared<DataTable>(fileEntry, mArchive);
                if (!gWorldLayer->initialLoadIDs()) gWorldLayer = nullptr;
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

    uint32 sig = r.u32le();
    uint32 ver = r.u32le();

    // Check for 'area' signature (little-endian)
    if (sig != 0x61726561u && sig != 0x41524541u) return false;

    // Find CHNK chunk
    std::vector<uint8> chnkData;
    while (r.can(8)) {
        uint32 magic = r.u32le();
        uint32 size = r.u32le();
        if (!r.can(size)) break;
        if (magic == 0x43484e4Bu || magic == 0x4B4E4843u) { // 'CHNK'
            chnkData.resize(size);
            r.bytes(chnkData.data(), size);
        } else {
            r.skip(size);
        }
    }

    if (chnkData.empty()) {
        mMinBounds = glm::vec3(-100,0,-100);
        mMaxBounds = glm::vec3(100,100,100);
        return true;
    }

    mChunks.assign(256, nullptr);
    const uint8* cbase = chnkData.data();
    R cr{ cbase, cbase + chnkData.size() };

    uint32 validCount = 0;
    float totalH = 0.0f;
    uint32 lastIndex = 0;

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

        // Payload starts after the 4-byte flags
        std::vector<uint8> payload(cellData.begin() + 4, cellData.end());

        // Calculate base position for this cell within the area
        // Each cell is 16x16 vertices at 2 units per vertex = 32 units per cell
        // 16 cells per row = 512 units per area file
        float cellBaseX = (float)(index % 16) * 16.0f * UnitSize;
        float cellBaseZ = (float)(index / 16) * 16.0f * UnitSize;

        auto chunk = std::make_shared<AreaChunkRender>(flags, payload, cellBaseX, cellBaseZ, mArchive);
        mChunks[index] = chunk;

        if (chunk && chunk->hasHeightmap()) {
            totalH += chunk->getAverageHeight();
            validCount++;
            mMaxHeight = std::max(mMaxHeight, chunk->getMaxHeight());
            mMinBounds = glm::min(mMinBounds, chunk->getMinBounds());
            mMaxBounds = glm::max(mMaxBounds, chunk->getMaxBounds());
        }
    }

    if (validCount > 0) {
        mAverageHeight = totalH / (float)validCount;
    } else {
        mMinBounds = glm::vec3(0, 0, 0);
        mMaxBounds = glm::vec3(512, 50, 512);
    }

    loadTexture();
    return true;
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

    // Apply world position transform based on tile coordinates
    glm::mat4 worldModel(1.0f);
    worldModel = glm::translate(worldModel, mWorldOffset);
    worldModel = glm::rotate(worldModel, glm::radians(mGlobalRotation), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec4 selectionColor(0.0f, 1.0f, 0.0f, 1.0f);

    bool isAreaSelected = false;
    if (selectedChunk) {
        for (const auto& c : mChunks) if (c == selectedChunk) { isAreaSelected = true; break; }
    }

    if (u.model != -1) glUniformMatrix4fv(u.model, 1, GL_FALSE, &worldModel[0][0]);

    if (u.highlightColor != -1) {
        if (isAreaSelected) glUniform4fv(u.highlightColor, 1, &selectionColor[0]);
        else glUniform4fv(u.highlightColor, 1, &mBaseColor[0]);
    }

    for (auto& c : mChunks) {
        if (c) c->render();
    }
}

// ============================================================================
// AreaChunkRender Implementation
// ============================================================================

AreaChunkRender::AreaChunkRender(uint32 flags, const std::vector<uint8>& payload, float baseX, float baseZ, ArchivePtr archive)
    : mChunkData(payload)
    , mFlags(flags)
    , mMinBounds(std::numeric_limits<float>::max())
    , mMaxBounds(std::numeric_limits<float>::lowest())
    , mSplatTexture(0)
    , mColorMapTexture(0)
{
    // Must have at least heightmap, texture IDs, and blend values
    if (!hasHeightmap() || !hasTextureIds() || !hasBlendValues()) {
        return;
    }

#pragma pack(push, 1)
    struct HeightMap { uint16 data[19][19]; };
    struct BlendData { uint16 data[65][65]; };
    struct ColorData { uint16 data[65][65]; };
#pragma pack(pop)

    // Calculate offsets based on sequential data layout
    size_t ofsHeightmap = 0;
    size_t ofsTexIds = sizeof(HeightMap);                    // 722 bytes
    size_t ofsBlend = ofsTexIds + 4 * sizeof(uint32);        // +16 = 738 bytes
    size_t ofsColorMap = ofsBlend + sizeof(BlendData);       // +8450 = 9188 bytes

    // Verify we have enough data
    if (mChunkData.size() < ofsBlend + sizeof(BlendData)) {
        std::cout << "Chunk data too small: " << mChunkData.size() << " bytes\n";
        return;
    }

    HeightMap& hm = *(HeightMap*)(mChunkData.data() + ofsHeightmap);
    uint32* textureIds = (uint32*)(mChunkData.data() + ofsTexIds);
    BlendData& blend = *(BlendData*)(mChunkData.data() + ofsBlend);

    ColorData* colorData = nullptr;
    if (hasColorMap() && mChunkData.size() >= ofsColorMap + sizeof(ColorData)) {
        colorData = (ColorData*)(mChunkData.data() + ofsColorMap);
    }

    // Build index buffer (shared across all chunks)
    if (indices.empty()) {
        // Diamond pattern: 16x16 cells, each with 4 triangles (center point subdivision)
        indices.resize(16 * 16 * 4 * 3);

        for (uint32 i = 0; i < 16; ++i) {
            for (uint32 j = 0; j < 16; ++j) {
                uint32 tribase = (i * 16 + j) * 4 * 3;
                uint32 ibase = i * 33 + j;

                // Four triangles per cell, sharing center vertex
                // Top-left triangle
                indices[tribase + 0] = ibase;
                indices[tribase + 1] = ibase + 1;
                indices[tribase + 2] = ibase + 17;  // Center vertex

                // Top-right triangle
                indices[tribase + 3] = ibase + 1;
                indices[tribase + 4] = ibase + 34;
                indices[tribase + 5] = ibase + 17;

                // Bottom-right triangle
                indices[tribase + 6] = ibase + 34;
                indices[tribase + 7] = ibase + 33;
                indices[tribase + 8] = ibase + 17;

                // Bottom-left triangle
                indices[tribase + 9] = ibase + 33;
                indices[tribase + 10] = ibase;
                indices[tribase + 11] = ibase + 17;
            }
        }
    }

    // Build vertex buffer with heightmap data
    float totalHeight = 0.0f;
    uint32 validHeights = 0;

    mVertices.resize(19 * 19);

    for (int32 y = -1; y < 18; ++y) {
        for (int32 x = -1; x < 18; ++x) {
            // Read height with hole mask removed
            uint16 h = hm.data[y + 1][x + 1] & 0x7FFF;

            AreaVertex v{};

            // Position: 2 units between vertices, starting at baseX/baseZ
            v.x = baseX + (float)x * AreaFile::UnitSize;
            v.z = baseZ + (float)y * AreaFile::UnitSize;

            // Height: raw value / 8 - 2048 (sea level offset)
            v.y = -2048.0f + (float)h / 8.0f;

            // UV coordinates: map 0-16 range to 0-1 for proper texture tiling
            // The -1 and 17 vertices are edge vertices for normal calculation
            // Inner 17x17 grid (0-16) maps to 0.0-1.0
            v.u = (float)x / 16.0f;
            v.v = (float)y / 16.0f;

            if (v.y > mMaxHeight) mMaxHeight = v.y;
            totalHeight += v.y;
            validHeights++;

            glm::vec3 pos(v.x, v.y, v.z);
            mMinBounds = glm::min(mMinBounds, pos);
            mMaxBounds = glm::max(mMaxBounds, pos);

            mVertices[(y + 1) * 19 + x + 1] = v;
        }
    }

    if (validHeights > 0) {
        mAverageHeight = totalHeight / (float)validHeights;
    }

    // Load textures from WorldLayer database
    mTexScales = Vector4(1.0f);

    if (archive && gWorldLayer) {
        WorldLayerEntry layer;
        for (int t = 0; t < 4; ++t) {
            uint32 tid = textureIds[t];
            if (tid != 0 && gWorldLayer->getRowById(tid, layer)) {
                if (layer.colorMapPath && wcslen(layer.colorMapPath) > 0) {
                    GLuint glID = LoadTextureFromArchive(archive, layer.colorMapPath);
                    mLayerTextures.push_back(glID);

                    // Calculate texture scale from metersPerTexture
                    float scale = 1.0f;
                    if (layer.metersPerTexture > 0.1f) {
                        scale = 1.0f / (layer.metersPerTexture / 32.0f);
                    }
                    ((float*)&mTexScales)[t] = scale;
                } else {
                    mLayerTextures.push_back(0);
                }
            } else {
                mLayerTextures.push_back(0);
            }
        }
    }

    // Build blend/splat texture from blend data
    // Each uint16 contains 4 nibbles (4 bits each) for the 4 texture layers
    mBlendValues.resize(65 * 65);
    std::fill(mBlendValues.begin(), mBlendValues.end(), 0);

    for (uint32 i = 0; i < 4; ++i) {
        for (uint32 j = 0; j < 65 * 65; ++j) {
            uint16 val = blend.data[j / 65][j % 65];
            uint32 value = (val >> (i * 4)) & 0xF;  // Extract 4-bit value for layer i
            uint8 weight = (uint8)((value / 15.0f) * 255.0f);
            mBlendValues[j] |= (weight << (8 * i));
        }
    }

    glGenTextures(1, &mSplatTexture);
    glBindTexture(GL_TEXTURE_2D, mSplatTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 65, 65, 0, GL_RGBA, GL_UNSIGNED_BYTE, mBlendValues.data());

    // Build color map texture if present
    if (hasColorMap() && colorData) {
        std::vector<uint32> colorValues(65 * 65);
        for (uint32 j = 0; j < 65 * 65; ++j) {
            uint16 value = colorData->data[j / 65][j % 65];
            // RGB565 format
            uint32 r = value & 0x1F;
            uint32 g = (value >> 5) & 0x3F;
            uint32 b = (value >> 11) & 0x1F;
            r = (uint32)((r / 31.0f) * 255.0f);
            g = (uint32)((g / 63.0f) * 255.0f);
            b = (uint32)((b / 31.0f) * 255.0f);
            colorValues[j] = 0xFF000000 | (b << 16) | (g << 8) | r;
        }

        glGenTextures(1, &mColorMapTexture);
        glBindTexture(GL_TEXTURE_2D, mColorMapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 65, 65, 0, GL_RGBA, GL_UNSIGNED_BYTE, colorValues.data());
    }

    // Extend vertex buffer to include center vertices for diamond pattern
    extendBuffer();

    // Calculate normals and tangents
    calcNormals();
    calcTangentBitangent();

    // Create OpenGL buffers
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(AreaVertex), mVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), indices.data(), GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)0);

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, nx));

    // Tangent
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, tanx));

    // TexCoord
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, u));

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

void AreaChunkRender::render() {
    if (!mVAO) return;
    const auto& u = getUniforms();

    // Bind splat/blend texture
    glActiveTexture(GL_TEXTURE8);
    if (mSplatTexture != 0) {
        glBindTexture(GL_TEXTURE_2D, mSplatTexture);
    } else {
        glBindTexture(GL_TEXTURE_2D, gFallbackWhite);
    }
    if ((GLint)u.alphaTexture != -1) glUniform1i((GLint)u.alphaTexture, 8);

    // Bind layer textures
    for (size_t i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + (GLenum)i);
        if (i < mLayerTextures.size() && mLayerTextures[i] != 0) {
            glBindTexture(GL_TEXTURE_2D, mLayerTextures[i]);
        } else {
            glBindTexture(GL_TEXTURE_2D, gFallbackWhite);
        }
        if ((GLint)u.textures[i] != -1) glUniform1i((GLint)u.textures[i], (GLint)i);
    }

    // Set texture scales
    if ((GLint)u.texScale != -1) glUniform4fv((GLint)u.texScale, 1, &mTexScales[0]);

    // Bind normal textures (fallback to flat normal)
    for (int i = 0; i < 4; ++i) {
        if ((GLint)u.normalTextures[i] != -1) {
            glActiveTexture(GL_TEXTURE4 + i);
            glBindTexture(GL_TEXTURE_2D, gFallbackFlatNormal);
            glUniform1i((GLint)u.normalTextures[i], 4 + i);
        }
    }

    // Color map
    if (hasColorMap() && mColorMapTexture != 0) {
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, mColorMapTexture);
        if ((GLint)u.colorTexture != -1) glUniform1i((GLint)u.colorTexture, 9);
        if ((GLint)u.hasColorMap != -1) glUniform1i((GLint)u.hasColorMap, 1);
    } else {
        if ((GLint)u.hasColorMap != -1) glUniform1i((GLint)u.hasColorMap, 0);
    }

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AreaChunkRender::extendBuffer() {
    // Create final vertex buffer: 17x17 corner vertices + 16x16 center vertices
    std::vector<AreaVertex> vertices(17 * 17 + 16 * 16);
    mFullVertices = mVertices;

    // Copy inner 17x17 grid (skip outer edge used for normal calculation)
    for (uint32 i = 0; i < 17; ++i) {
        for (uint32 j = 0; j < 17; ++j) {
            auto idx = i * 33 + j;
            vertices[idx] = mVertices[(i + 1) * 19 + j + 1];
        }
    }

    // Create center vertices for diamond pattern (average of 4 corners)
    for (uint32 i = 0; i < 16; ++i) {
        for (uint32 j = 0; j < 16; ++j) {
            auto idx = 17 + i * 33 + j;
            auto& tl = mVertices[(i + 1) * 19 + j + 1];
            auto& tr = mVertices[(i + 1) * 19 + j + 2];
            auto& bl = mVertices[(i + 2) * 19 + j + 1];
            auto& br = mVertices[(i + 2) * 19 + j + 2];

            AreaVertex v{};
            v.x = (tl.x + tr.x + bl.x + br.x) / 4.0f;
            v.y = (tl.y + tr.y + bl.y + br.y) / 4.0f;
            v.z = (tl.z + tr.z + bl.z + br.z) / 4.0f;
            v.u = (tl.u + tr.u) / 2.0f;
            v.v = (tl.v + bl.v) / 2.0f;

            vertices[idx] = v;
        }
    }

    mVertices = std::move(vertices);
}

void AreaChunkRender::calcNormals() {
    // Calculate normals for corner vertices using 4-neighbor cross products
    for (uint32 i = 1; i < 18; ++i) {
        for (uint32 j = 1; j < 18; ++j) {
            auto& tl = mFullVertices[(i - 1) * 19 + j - 1];
            auto& tr = mFullVertices[(i - 1) * 19 + j + 1];
            auto& br = mFullVertices[(i + 1) * 19 + j + 1];
            auto& bl = mFullVertices[(i + 1) * 19 + j - 1];
            auto& v = mFullVertices[i * 19 + j];

            glm::vec3 P1(tl.x, tl.y, tl.z);
            glm::vec3 P2(tr.x, tr.y, tr.z);
            glm::vec3 P3(br.x, br.y, br.z);
            glm::vec3 P4(bl.x, bl.y, bl.z);
            glm::vec3 vert(v.x, v.y, v.z);

            auto N1 = glm::cross(P2 - vert, P1 - vert);
            auto N2 = glm::cross(P3 - vert, P2 - vert);
            auto N3 = glm::cross(P4 - vert, P3 - vert);
            auto N4 = glm::cross(P1 - vert, P4 - vert);

            auto norm = glm::normalize(-(N1 + N2 + N3 + N4));

            auto& vnew = mVertices[(i - 1) * 33 + j - 1];
            vnew.nx = norm.x;
            vnew.ny = norm.y;
            vnew.nz = norm.z;
        }
    }

    // Average normals for center vertices
    for (uint32 i = 0; i < 16; ++i) {
        for (uint32 j = 0; j < 16; ++j) {
            auto idx = 17 + i * 33 + j;
            auto& tl = mVertices[i * 33 + j];
            auto& tr = mVertices[i * 33 + j + 1];
            auto& bl = mVertices[(i + 1) * 33 + j];
            auto& br = mVertices[(i + 1) * 33 + j + 1];

            auto& v = mVertices[idx];
            v.nx = (tl.nx + tr.nx + bl.nx + br.nx) / 4.0f;
            v.ny = (tl.ny + tr.ny + bl.ny + br.ny) / 4.0f;
            v.nz = (tl.nz + tr.nz + bl.nz + br.nz) / 4.0f;
        }
    }
}

void AreaChunkRender::calcTangentBitangent() {
    auto triCount = indices.size() / 3;
    std::vector<glm::vec3> tan1(mVertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(mVertices.size(), glm::vec3(0.0f));

    for (uint32 i = 0; i < triCount; ++i) {
        auto i1 = indices[i * 3];
        auto i2 = indices[i * 3 + 1];
        auto i3 = indices[i * 3 + 2];

        if (i1 >= mVertices.size() || i2 >= mVertices.size() || i3 >= mVertices.size()) continue;

        auto& v1 = mVertices[i1];
        auto& v2 = mVertices[i2];
        auto& v3 = mVertices[i3];

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;

        float s1 = v2.u - v1.u;
        float s2 = v3.u - v1.u;
        float t1 = v2.v - v1.v;
        float t2 = v3.v - v1.v;

        float det = s1 * t2 - s2 * t1;
        if (std::fabs(det) < 1e-8f) continue;
        float r = 1.0f / det;

        glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan1[i3] += sdir;

        tan2[i1] += tdir;
        tan2[i2] += tdir;
        tan2[i3] += tdir;
    }

    for (uint32 i = 0; i < mVertices.size(); ++i) {
        auto& v = mVertices[i];
        glm::vec3 n(v.nx, v.ny, v.nz);
        glm::vec3& t = tan1[i];
        glm::vec3& t2 = tan2[i];

        if (glm::length(t) < 1e-8f) {
            v.tanx = 1.0f; v.tany = 0.0f; v.tanz = 0.0f; v.tanw = 1.0f;
            continue;
        }

        glm::vec3 tan = glm::normalize(t - n * glm::dot(n, t));
        float tanw = glm::dot(glm::cross(n, t), t2) < 0.0f ? 1.0f : -1.0f;

        v.tanx = tan.x;
        v.tany = tan.y;
        v.tanz = tan.z;
        v.tanw = tanw;
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