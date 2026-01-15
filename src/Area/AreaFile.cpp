#include "AreaFile.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <zlib.h>
#include <memory>
#include <cstring>
#include <limits>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
#include "../tex/tex.h"

const float AreaFile::UnitSize = 2.0f;
const float AreaFile::GRID_SIZE = 512.0f;
static DataTablePtr gWorldLayer = nullptr;

static int gReferenceTileX = -1;
static int gReferenceTileY = -1;

std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

static const int kCellDataSize[32] = {
    722, 16, 8450, 8450, 8450, 4, 64, 16,
    4225, 2178, 4, 578, 1, 4624, 2312, 8450,
    4096, 2312, 2312, 2312, 2312, 1, 16, 16900,
    8, 8450, 21316, 4096, 16, 8450, 8450, 2312
};

const AreaChunkRender::Uniforms& AreaChunkRender::getUniforms()
{
    return uniforms;
}

void ResetAreaReferencePosition()
{
    gReferenceTileX = -1;
    gReferenceTileY = -1;
    std::cout << "Reset reference tile position\n";
}

static inline int hexNibble(wchar_t c) {
    if (c >= L'0' && c <= L'9') return static_cast<int>(c - L'0');
    if (c >= L'a' && c <= L'f') return 10 + static_cast<int>(c - L'a');
    if (c >= L'A' && c <= L'F') return 10 + static_cast<int>(c - L'A');
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

static GLuint LoadTextureFromArchive(const ArchivePtr& archive, const std::wstring& fullPath)
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

void AreaFile::calculateWorldOffset()
{
    if (gReferenceTileX < 0 || gReferenceTileY < 0) {
        gReferenceTileX = mTileX;
        gReferenceTileY = mTileY;
    }

    mWorldOffset.x = static_cast<float>(mTileX - gReferenceTileX) * GRID_SIZE;
    mWorldOffset.y = 0.0f;
    mWorldOffset.z = static_cast<float>(mTileY - gReferenceTileY) * GRID_SIZE;
}

AreaFile::AreaFile(ArchivePtr archive, FileEntryPtr file)
    : mArchive(std::move(archive))
    , mFile(std::move(file))
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
            calculateWorldOffset();
        } catch (...) {
        }
    }
}

AreaFile::~AreaFile() {
    if (mTextureID != 0) glDeleteTextures(1, &mTextureID);
}

bool AreaFile::loadTexture() {
    if (!mFile || !mArchive) return false;
    std::wstring areaName = mFile->getEntryName();
    std::wstring texName = ReplaceExtension(areaName, L".tex");

    if(auto parent = mFile->getParent()) {
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
    if (mContent.size() < 8) {
        return false;
    }

    if (gWorldLayer == nullptr && mArchive) {
        if (auto genericEntry = mArchive->getByPath(L"DB\\WorldLayer.tbl")) {
            if (auto fileEntry = std::dynamic_pointer_cast<FileEntry>(genericEntry)) {
                gWorldLayer = std::make_shared<DataTable>(fileEntry, mArchive);
                if (!gWorldLayer->initialLoadIDs()) {
                    gWorldLayer = nullptr;
                }
            }
        }
    }

    struct R {
        const uint8* p;
        const uint8* e;
        bool can(size_t n) const { return static_cast<size_t>(e - p) >= n; }
        uint32 u32le() {
            uint32 v = static_cast<uint32>(p[0]) |
                      (static_cast<uint32>(p[1]) << 8) |
                      (static_cast<uint32>(p[2]) << 16) |
                      (static_cast<uint32>(p[3]) << 24);
            p += 4;
            return v;
        }
        uint16 u16le() {
            uint16 v = static_cast<uint16>(p[0]) | (static_cast<uint16>(p[1]) << 8);
            p += 2;
            return v;
        }
        void bytes(void* out, size_t n) { memcpy(out, p, n); p += n; }
        void skip(size_t n) { p += n; }
        size_t remaining() const { return static_cast<size_t>(e - p); }
    };

    const uint8* base = mContent.data();
    R r{ base, base + mContent.size() };

    uint32 sig = r.u32le();

    if (sig != 0x61726561u && sig != 0x41524541u) {
        return false;
    }

    uint32 version = r.u32le();

    std::vector<uint8> chnkData;

    while (r.can(8)) {
        uint32 magic = r.u32le();
        uint32 size = r.u32le();
        if (!r.can(size)) break;

        if (magic == 0x43484E4Bu) {
            chnkData.resize(size);
            r.bytes(chnkData.data(), size);
        } else {
            r.skip(size);
        }
    }

    if (chnkData.empty()) {
        mMinBounds = glm::vec3(0, 0, 0);
        mMaxBounds = glm::vec3(512, 50, 512);
        return true;
    }

    mChunks.assign(256, nullptr);
    R cr{ chnkData.data(), chnkData.data() + chnkData.size() };

    uint32 validCount = 0;
    float totalH = 0.0f;
    uint32 lastIndex = 0;

    while (cr.can(4)) {
        uint32 cellInfo = cr.u32le();
        uint32 idxDelta = (cellInfo >> 24) & 0xFF;
        uint32 size = cellInfo & 0x00FFFFFF;

        uint32 index = idxDelta + lastIndex;
        lastIndex = index + 1;

        if (index >= 256) break;
        if (!cr.can(size)) break;
        if (size < 4) {
            cr.skip(size);
            continue;
        }

        std::vector<uint8> cellData(size);
        cr.bytes(cellData.data(), size);

        float cellBaseX = static_cast<float>(index % 16) * 16.0f * UnitSize;
        float cellBaseZ = static_cast<float>(index / 16) * 16.0f * UnitSize;

        auto chunk = std::make_shared<AreaChunkRender>(cellData, cellBaseX, cellBaseZ, mArchive);
        mChunks[index] = chunk;

        if (chunk && chunk->isFullyInitialized()) {
            totalH += chunk->getAverageHeight();
            validCount++;
            mMaxHeight = std::max(mMaxHeight, chunk->getMaxHeight());
            mMinBounds = glm::min(mMinBounds, chunk->getMinBounds());
            mMaxBounds = glm::max(mMaxBounds, chunk->getMaxBounds());
        }
    }

    if (validCount > 0) {
        mAverageHeight = totalH / static_cast<float>(validCount);
    } else {
        mMinBounds = glm::vec3(0, 0, 0);
        mMaxBounds = glm::vec3(512, 50, 512);
    }

    loadTexture();
    return true;
}

void AreaFile::render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram, const AreaChunkRenderPtr& selectedChunk)
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
    if (static_cast<GLint>(u.camPosition) != -1) glUniform3f(static_cast<GLint>(u.camPosition), 0.0f, 0.0f, 0.0f);

    glm::mat4 worldModel(1.0f);
    worldModel = glm::translate(worldModel, mWorldOffset);
    worldModel = glm::rotate(worldModel, glm::radians(mGlobalRotation), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec4 selectionColor(0.0f, 1.0f, 0.0f, 1.0f);

    bool isAreaSelected = false;
    if (selectedChunk) {
        for (const auto& c : mChunks) if (c == selectedChunk) { isAreaSelected = true; break; }
    }

    if (u.model != static_cast<uint32>(-1)) glUniformMatrix4fv(u.model, 1, GL_FALSE, &worldModel[0][0]);

    if (u.highlightColor != static_cast<uint32>(-1)) {
        if (isAreaSelected) glUniform4fv(u.highlightColor, 1, &selectionColor[0]);
        else glUniform4fv(u.highlightColor, 1, &mBaseColor[0]);
    }

    if (u.baseColor != static_cast<uint32>(-1)) {
        glUniform4f(u.baseColor, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    for (auto& c : mChunks) {
        if (c && c->isFullyInitialized()) c->render();
    }
}

AreaChunkRender::AreaChunkRender(const std::vector<uint8>& cellData, float baseX, float baseZ, ArchivePtr archive)
    : mChunkData(cellData)
    , mFlags(0)
    , mMinBounds(std::numeric_limits<float>::max())
    , mMaxBounds(std::numeric_limits<float>::lowest())
    , mSplatTexture(0)
    , mColorMapTexture(0)
{
    if (cellData.size() < 4) return;

    struct R {
        const uint8* p;
        const uint8* e;
        bool can(size_t n) const { return static_cast<size_t>(e - p) >= n; }
        uint32 u32le() {
            uint32 v = static_cast<uint32>(p[0]) |
                      (static_cast<uint32>(p[1]) << 8) |
                      (static_cast<uint32>(p[2]) << 16) |
                      (static_cast<uint32>(p[3]) << 24);
            p += 4;
            return v;
        }
        uint16 u16le() {
            uint16 v = static_cast<uint16>(p[0]) | (static_cast<uint16>(p[1]) << 8);
            p += 2;
            return v;
        }
        uint8 u8() { return *p++; }
        void skip(size_t n) { p += n; }
        size_t remaining() const { return static_cast<size_t>(e - p); }
        const uint8* current() const { return p; }
    };

    R r{ cellData.data(), cellData.data() + cellData.size() };

    mFlags = r.u32le();

    std::vector<uint16> heightMap;
    uint32 worldLayerIDs[4] = {0};
    std::vector<uint8> blendMapRGBA(65 * 65 * 4, 0);
    bool hasBlendData = false;
    std::vector<uint8> colorMapRGBA;

    for (int i = 0; i < 32; i++) {
        uint32 flag = 1u << i;
        if ((mFlags & flag) == 0) continue;

        if (!r.can(kCellDataSize[i])) {
            break;
        }

        switch (i) {
            case 0:
            {
                heightMap.resize(19 * 19);
                for (int j = 0; j < 19 * 19; j++) {
                    heightMap[j] = r.u16le();
                }
                break;
            }

            case 1:
            {
                for (int j = 0; j < 4; j++) {
                    worldLayerIDs[j] = r.u32le();
                }
                mWorldLayerIDs[0] = worldLayerIDs[0];
                mWorldLayerIDs[1] = worldLayerIDs[1];
                mWorldLayerIDs[2] = worldLayerIDs[2];
                mWorldLayerIDs[3] = worldLayerIDs[3];
                break;
            }

            case 2:
            {
                hasBlendData = true;
                for (int j = 0; j < 65 * 65; j++) {
                    uint16 val = r.u16le();
                    blendMapRGBA[j * 4 + 0] = static_cast<uint8>(((val >> 0) & 0xF) * 17);
                    blendMapRGBA[j * 4 + 1] = static_cast<uint8>(((val >> 4) & 0xF) * 17);
                    blendMapRGBA[j * 4 + 2] = static_cast<uint8>(((val >> 8) & 0xF) * 17);
                    blendMapRGBA[j * 4 + 3] = static_cast<uint8>(((val >> 12) & 0xF) * 17);
                }
                break;
            }

            case 3:
            {
                colorMapRGBA.resize(65 * 65 * 4);
                for (int j = 0; j < 65 * 65; j++) {
                    uint16 val = r.u16le();
                    uint32 red = (val & 0x1F);
                    uint32 green = ((val >> 5) & 0x3F);
                    uint32 blue = ((val >> 11) & 0x1F);
                    colorMapRGBA[j * 4 + 0] = static_cast<uint8>((red * 255) / 31);
                    colorMapRGBA[j * 4 + 1] = static_cast<uint8>((green * 255) / 63);
                    colorMapRGBA[j * 4 + 2] = static_cast<uint8>((blue * 255) / 31);
                    colorMapRGBA[j * 4 + 3] = 255;
                }
                break;
            }

            case 16:
            {
                r.skip(4096);
                break;
            }

            case 17:
            {
                hasBlendData = true;
                const uint8* dxtData = r.current();
                r.skip(2312);

                auto decodeBlock = [](const uint8* block, uint8* outRGBA, int pitch) {
                    uint16 c0 = block[0] | (block[1] << 8);
                    uint16 c1 = block[2] | (block[3] << 8);

                    uint8 colors[4][4];
                    colors[0][0] = ((c0 >> 11) & 0x1F) * 255 / 31;
                    colors[0][1] = ((c0 >> 5) & 0x3F) * 255 / 63;
                    colors[0][2] = (c0 & 0x1F) * 255 / 31;
                    colors[0][3] = 255;

                    colors[1][0] = ((c1 >> 11) & 0x1F) * 255 / 31;
                    colors[1][1] = ((c1 >> 5) & 0x3F) * 255 / 63;
                    colors[1][2] = (c1 & 0x1F) * 255 / 31;
                    colors[1][3] = 255;

                    if (c0 > c1) {
                        colors[2][0] = (2 * colors[0][0] + colors[1][0]) / 3;
                        colors[2][1] = (2 * colors[0][1] + colors[1][1]) / 3;
                        colors[2][2] = (2 * colors[0][2] + colors[1][2]) / 3;
                        colors[2][3] = 255;
                        colors[3][0] = (colors[0][0] + 2 * colors[1][0]) / 3;
                        colors[3][1] = (colors[0][1] + 2 * colors[1][1]) / 3;
                        colors[3][2] = (colors[0][2] + 2 * colors[1][2]) / 3;
                        colors[3][3] = 255;
                    } else {
                        colors[2][0] = (colors[0][0] + colors[1][0]) / 2;
                        colors[2][1] = (colors[0][1] + colors[1][1]) / 2;
                        colors[2][2] = (colors[0][2] + colors[1][2]) / 2;
                        colors[2][3] = 255;
                        colors[3][0] = 0;
                        colors[3][1] = 0;
                        colors[3][2] = 0;
                        colors[3][3] = 0;
                    }

                    uint32 indices = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);
                    for (int y = 0; y < 4; y++) {
                        for (int x = 0; x < 4; x++) {
                            int idx = (indices >> ((y * 4 + x) * 2)) & 0x3;
                            uint8* dst = outRGBA + y * pitch + x * 4;
                            dst[0] = colors[idx][0];
                            dst[1] = colors[idx][1];
                            dst[2] = colors[idx][2];
                            dst[3] = colors[idx][3];
                        }
                    }
                };

                int blockWidth = (65 + 3) / 4;
                int blockHeight = (65 + 3) / 4;

                for (int by = 0; by < blockHeight; by++) {
                    for (int bx = 0; bx < blockWidth; bx++) {
                        int blockIdx = by * blockWidth + bx;
                        if (blockIdx * 8 >= 2312) break;

                        uint8 blockRGBA[4 * 4 * 4];
                        decodeBlock(dxtData + blockIdx * 8, blockRGBA, 4 * 4);

                        for (int y = 0; y < 4; y++) {
                            for (int x = 0; x < 4; x++) {
                                int outX = bx * 4 + x;
                                int outY = by * 4 + y;
                                if (outX >= 65 || outY >= 65) continue;

                                int srcIdx = (y * 4 + x) * 4;
                                int dstIdx = (outY * 65 + outX) * 4;
                                blendMapRGBA[dstIdx + 0] = blockRGBA[srcIdx + 0];
                                blendMapRGBA[dstIdx + 1] = blockRGBA[srcIdx + 1];
                                blendMapRGBA[dstIdx + 2] = blockRGBA[srcIdx + 2];
                                blendMapRGBA[dstIdx + 3] = blockRGBA[srcIdx + 3];
                            }
                        }
                    }
                }
                break;
            }

            case 28:
            {
                for (int j = 0; j < 4; j++) {
                    mZoneIds[j] = r.u32le();
                }
                break;
            }

            default:
                r.skip(kCellDataSize[i]);
                break;
        }
    }

    while (r.can(8)) {
        uint32 subMagic = r.u32le();
        uint32 subSize = r.u32le();
        if (!r.can(subSize)) break;
        r.skip(subSize);
    }

    if (heightMap.empty()) {
        return;
    }

    float totalHeight = 0.0f;
    uint32 validHeights = 0;
    mVertices.resize(19 * 19);

    for (int32 y = 0; y < 19; ++y) {
        for (int32 x = 0; x < 19; ++x) {
            uint16 h = heightMap[y * 19 + x] & 0x7FFF;

            AreaVertex v{};
            v.x = baseX + static_cast<float>(x - 1) * AreaFile::UnitSize;
            v.z = baseZ + static_cast<float>(y - 1) * AreaFile::UnitSize;
            v.y = -2048.0f + static_cast<float>(h) / 8.0f;
            v.u = static_cast<float>(x) / 17.0f;
            v.v = static_cast<float>(y) / 17.0f;

            if (v.y > mMaxHeight) mMaxHeight = v.y;
            totalHeight += v.y;
            validHeights++;

            glm::vec3 pos(v.x, v.y, v.z);
            mMinBounds = glm::min(mMinBounds, pos);
            mMaxBounds = glm::max(mMaxBounds, pos);

            mVertices[y * 19 + x] = v;
        }
    }

    if (validHeights > 0) {
        mAverageHeight = totalHeight / static_cast<float>(validHeights);
    }

    if (!hasBlendData) {
        for (int i = 0; i < 65 * 65 * 4; i += 4) {
            blendMapRGBA[i + 0] = 255;
            blendMapRGBA[i + 1] = 0;
            blendMapRGBA[i + 2] = 0;
            blendMapRGBA[i + 3] = 0;
        }
    }

    glGenTextures(1, &mSplatTexture);
    glBindTexture(GL_TEXTURE_2D, mSplatTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 65, 65, 0, GL_RGBA, GL_UNSIGNED_BYTE, blendMapRGBA.data());

    if (!colorMapRGBA.empty()) {
        glGenTextures(1, &mColorMapTexture);
        glBindTexture(GL_TEXTURE_2D, mColorMapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 65, 65, 0, GL_RGBA, GL_UNSIGNED_BYTE, colorMapRGBA.data());
    }

    mLayerTextures.resize(4, 0);
    mTexScales = glm::vec4(8.0f);

    if (gWorldLayer && hasWorldLayerIDs()) {
        for (int i = 0; i < 4; i++) {
            if (mWorldLayerIDs[i] == 0) continue;

            WorldLayerEntry entry{};
            if (gWorldLayer->getRowById(mWorldLayerIDs[i], entry)) {
                if (entry.colorMapPath && wcslen(entry.colorMapPath) > 0) {
                    mLayerTextures[i] = LoadTextureFromArchive(archive, entry.colorMapPath);
                }
                if (entry.metersPerTexture > 0.0f) {
                    mTexScales[i] = 32.0f / entry.metersPerTexture;
                }
            }
        }
    }

    if (indices.empty()) {
        indices.resize(16 * 16 * 4 * 3);

        for (uint32 i = 0; i < 16; ++i) {
            for (uint32 j = 0; j < 16; ++j) {
                uint32 tribase = (i * 16 + j) * 4 * 3;
                uint32 ibase = i * 33 + j;

                indices[tribase + 0] = ibase;
                indices[tribase + 1] = ibase + 1;
                indices[tribase + 2] = ibase + 17;

                indices[tribase + 3] = ibase + 1;
                indices[tribase + 4] = ibase + 34;
                indices[tribase + 5] = ibase + 17;

                indices[tribase + 6] = ibase + 34;
                indices[tribase + 7] = ibase + 33;
                indices[tribase + 8] = ibase + 17;

                indices[tribase + 9] = ibase + 33;
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
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, tanx));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, u));

    glBindVertexArray(0);
    mIndexCount = static_cast<int>(indices.size());
}

AreaChunkRender::~AreaChunkRender() {
    if (mSplatTexture != 0) glDeleteTextures(1, &mSplatTexture);
    if (mColorMapTexture != 0) glDeleteTextures(1, &mColorMapTexture);
    for (auto tex : mLayerTextures) {
        if (tex != 0) glDeleteTextures(1, &tex);
    }
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
    if (mEBO) glDeleteBuffers(1, &mEBO);
}

void AreaChunkRender::render() {
    if (!mVAO) return;

    const auto& u = getUniforms();

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, mSplatTexture ? mSplatTexture : gFallbackWhite);
    if (static_cast<GLint>(u.alphaTexture) != -1) glUniform1i(static_cast<GLint>(u.alphaTexture), 8);

    for (size_t i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
        glBindTexture(GL_TEXTURE_2D, (i < mLayerTextures.size() && mLayerTextures[i]) ? mLayerTextures[i] : gFallbackWhite);
        if (static_cast<GLint>(u.textures[i]) != -1) glUniform1i(static_cast<GLint>(u.textures[i]), static_cast<GLint>(i));
    }

    if (static_cast<GLint>(u.texScale) != -1) glUniform4fv(static_cast<GLint>(u.texScale), 1, &mTexScales[0]);

    for (int i = 0; i < 4; ++i) {
        if (static_cast<GLint>(u.normalTextures[i]) != -1) {
            glActiveTexture(GL_TEXTURE4 + i);
            glBindTexture(GL_TEXTURE_2D, gFallbackFlatNormal);
            glUniform1i(static_cast<GLint>(u.normalTextures[i]), 4 + i);
        }
    }

    if ((hasColorMap() || hasColorMapDXT()) && mColorMapTexture != 0) {
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, mColorMapTexture);
        if (static_cast<GLint>(u.colorTexture) != -1) glUniform1i(static_cast<GLint>(u.colorTexture), 9);
        if (static_cast<GLint>(u.hasColorMap) != -1) glUniform1i(static_cast<GLint>(u.hasColorMap), 1);
    } else {
        if (static_cast<GLint>(u.hasColorMap) != -1) glUniform1i(static_cast<GLint>(u.hasColorMap), 0);
    }

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AreaChunkRender::extendBuffer() {
    std::vector<AreaVertex> vertices(17 * 17 + 16 * 16);
    mFullVertices = mVertices;

    for (uint32 i = 0; i < 17; ++i) {
        for (uint32 j = 0; j < 17; ++j) {
            vertices[i * 33 + j] = mVertices[(i + 1) * 19 + j + 1];
        }
    }

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

        float x1 = v2.x - v1.x, x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y, y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z, z2 = v3.z - v1.z;
        float s1 = v2.u - v1.u, s2 = v3.u - v1.u;
        float t1 = v2.v - v1.v, t2 = v3.v - v1.v;

        float det = s1 * t2 - s2 * t1;
        if (std::fabs(det) < 1e-8f) continue;
        float r = 1.0f / det;

        glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

        tan1[i1] += sdir; tan1[i2] += sdir; tan1[i3] += sdir;
        tan2[i1] += tdir; tan2[i2] += tdir; tan2[i3] += tdir;
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

        v.tanx = tan.x; v.tany = tan.y; v.tanz = tan.z; v.tanw = tanw;
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
    uniforms.baseColor    = glGetUniformLocation(program, "baseColor");
    for (int i = 0; i < 4; ++i) {
        std::string t = "textures[" + std::to_string(i) + "]";
        std::string n = "normalTextures[" + std::to_string(i) + "]";
        uniforms.textures[i]       = glGetUniformLocation(program, t.c_str());
        uniforms.normalTextures[i] = glGetUniformLocation(program, n.c_str());
    }
}