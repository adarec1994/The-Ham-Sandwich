#include "AreaFile.h"
#include "TerrainTexture.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <memory>
#include <cstring>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

const float AreaFile::UnitSize = 2.0f;
const float AreaFile::GRID_SIZE = 512.0f;

static int gReferenceTileX = -1;
static int gReferenceTileY = -1;

std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

const AreaChunkRender::Uniforms& AreaChunkRender::getUniforms()
{
    return uniforms;
}

void ResetAreaReferencePosition()
{
    gReferenceTileX = -1;
    gReferenceTileY = -1;
}

static inline int hexNibble(wchar_t c) {
    if (c >= L'0' && c <= L'9') return static_cast<int>(c - L'0');
    if (c >= L'a' && c <= L'f') return 10 + static_cast<int>(c - L'a');
    if (c >= L'A' && c <= L'F') return 10 + static_cast<int>(c - L'A');
    return -1;
}

static inline bool parseHexByte(const std::wstring& s, size_t pos, int& outByte) {
    if (pos + 2 > s.size()) return false;
    int hi = hexNibble(s[pos]);
    int lo = hexNibble(s[pos + 1]);
    if (hi < 0 || lo < 0) return false;
    outByte = (hi << 4) | lo;
    return true;
}

static GLuint gFallbackWhite = 0;
static GLuint gFallbackNormal = 0;
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
    if (gFallbackNormal == 0) {
        uint8_t px[4] = { 128, 128, 255, 255 };
        glGenTextures(1, &gFallbackNormal);
        glBindTexture(GL_TEXTURE_2D, gFallbackNormal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
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
    mTileX = bx;
    mTileY = by;
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
    return false;
}

bool AreaFile::load()
{
    if (mContent.size() < 8) {
        return false;
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
        void bytes(void* out, size_t n) { memcpy(out, p, n); p += n; }
        void skip(size_t n) { p += n; }
    };

    const uint8* base = mContent.data();
    R r{ base, base + mContent.size() };

    uint32 sig = r.u32le();
    if (sig != 0x61726561u && sig != 0x41524541u) {
        return false;
    }

    r.u32le();

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

        uint32 cellX = index % 16;
        uint32 cellY = index / 16;

        auto chunk = std::make_shared<AreaChunkRender>(cellData, cellX, cellY, mArchive);
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

    glm::mat4 worldModel(1.0f);
    worldModel = glm::translate(worldModel, mWorldOffset);
    worldModel = glm::rotate(worldModel, glm::radians(mGlobalRotation), glm::vec3(0.0f, 1.0f, 0.0f));

    const auto& u = AreaChunkRender::getUniforms();

    if (u.model != static_cast<uint32>(-1)) glUniformMatrix4fv(u.model, 1, GL_FALSE, &worldModel[0][0]);
    if (u.baseColor != static_cast<uint32>(-1)) glUniform4f(u.baseColor, 1.0f, 1.0f, 1.0f, 1.0f);

    for (auto& c : mChunks) {
        if (!c || !c->isFullyInitialized()) continue;

        c->loadTextures(mArchive);

        GLint highlightLoc = glGetUniformLocation(shaderProgram, "highlightColor");
        if (highlightLoc != -1) {
            if (c == selectedChunk)
                glUniform4f(highlightLoc, 1.0f, 1.0f, 0.0f, 0.5f);
            else
                glUniform4f(highlightLoc, 0.0f, 0.0f, 0.0f, 0.0f);
        }

        c->bindTextures(shaderProgram);
        c->render();
    }
}

void AreaChunkRender::loadTextures(const ArchivePtr& archive)
{
    if (mTexturesLoaded) return;

    auto& texMgr = TerrainTexture::Manager::Instance();

    texMgr.LoadWorldLayerTable(archive);

    if (!mBlendMap.empty())
    {
        mBlendMapTexture = texMgr.CreateBlendMapTexture(mBlendMap.data(), 65, 65);
    }
    else if (!mBlendMapDXT.empty())
    {
        mBlendMapTexture = texMgr.CreateBlendMapFromDXT1(mBlendMapDXT.data(), mBlendMapDXT.size(), 65, 65);
    }
    else
    {
        uint8_t defaultBlend[4] = {255, 0, 0, 0};
        mBlendMapTexture = texMgr.CreateBlendMapTexture(defaultBlend, 1, 1);
    }

    if (!mColorMap.empty())
    {
        mColorMapTextureGPU = texMgr.CreateColorMapTexture(mColorMap.data(), 65, 65);
    }
    else if (!mColorMapDXT.empty())
    {
        mColorMapTextureGPU = texMgr.CreateColorMapFromDXT5(mColorMapDXT.data(), mColorMapDXT.size(), 65, 65);
    }

    for (int i = 0; i < 4; ++i)
    {
        uint32_t layerId = mWorldLayerIDs[i];
        if (layerId == 0)
        {
            mLayerDiffuse[i] = gFallbackWhite;
            mLayerNormal[i] = gFallbackNormal;
            mLayerScale[i] = 4.0f;
            continue;
        }

        const auto* cached = texMgr.GetLayerTexture(archive, layerId);
        if (cached && cached->loaded)
        {
            mLayerDiffuse[i] = cached->diffuse;
            mLayerNormal[i] = cached->normal ? cached->normal : gFallbackNormal;
        }
        else
        {
            mLayerDiffuse[i] = gFallbackWhite;
            mLayerNormal[i] = gFallbackNormal;
        }

        const auto* layerEntry = texMgr.GetLayerEntry(layerId);
        if (layerEntry && layerEntry->scaleU > 0.0f)
        {
            mLayerScale[i] = layerEntry->scaleU;
        }
        else
        {
            mLayerScale[i] = 4.0f;
        }
    }

    mTexturesLoaded = true;
}

void AreaChunkRender::bindTextures(unsigned int program) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mBlendMapTexture ? mBlendMapTexture : gFallbackWhite);
    glUniform1i(glGetUniformLocation(program, "blendMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mColorMapTextureGPU ? mColorMapTextureGPU : gFallbackWhite);
    glUniform1i(glGetUniformLocation(program, "colorMap"), 1);
    glUniform1i(glGetUniformLocation(program, "hasColorMap"), mColorMapTextureGPU ? 1 : 0);

    for (int i = 0; i < 4; ++i)
    {
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, mLayerDiffuse[i] ? mLayerDiffuse[i] : gFallbackWhite);

        const char* names[] = {"layer0", "layer1", "layer2", "layer3"};
        glUniform1i(glGetUniformLocation(program, names[i]), 2 + i);
    }

    for (int i = 0; i < 4; ++i)
    {
        glActiveTexture(GL_TEXTURE6 + i);
        glBindTexture(GL_TEXTURE_2D, mLayerNormal[i] ? mLayerNormal[i] : gFallbackNormal);

        const char* names[] = {"layer0Normal", "layer1Normal", "layer2Normal", "layer3Normal"};
        glUniform1i(glGetUniformLocation(program, names[i]), 6 + i);
    }

    float s0 = mLayerScale[0] > 0.0f ? 32.0f / mLayerScale[0] : 8.0f;
    float s1 = mLayerScale[1] > 0.0f ? 32.0f / mLayerScale[1] : 8.0f;
    float s2 = mLayerScale[2] > 0.0f ? 32.0f / mLayerScale[2] : 8.0f;
    float s3 = mLayerScale[3] > 0.0f ? 32.0f / mLayerScale[3] : 8.0f;
    glUniform4f(glGetUniformLocation(program, "texScale"), s0, s1, s2, s3);

    glActiveTexture(GL_TEXTURE0);
}

AreaChunkRender::AreaChunkRender(const std::vector<uint8>& cellData, uint32 cellX, uint32 cellY, ArchivePtr)
    : mChunkData(cellData)
    , mFlags(0)
    , mMinBounds(std::numeric_limits<float>::max())
    , mMaxBounds(std::numeric_limits<float>::lowest())
    , mSplatTexture(0)
    , mColorMapTexture(0)
{
    for (int i = 0; i < 4; ++i) mLayerScale[i] = 0.0f;

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
    };

    R r{ cellData.data(), cellData.data() + cellData.size() };

    mFlags = r.u32le();

    std::vector<uint16> heightmap;
    bool hasHeightmap = false;

    if ((mFlags & ChnkCellFlags::HeightMap) && r.can(722)) {
        hasHeightmap = true;
        heightmap.resize(19 * 19);
        for (int j = 0; j < 19 * 19; j++) {
            heightmap[j] = r.u16le();
        }
    }

    if ((mFlags & ChnkCellFlags::WorldLayerIDs) && r.can(16)) {
        for (int j = 0; j < 4; j++) {
            mWorldLayerIDs[j] = r.u32le();
        }
    }

    if ((mFlags & ChnkCellFlags::BlendMap) && r.can(8450)) {
        mBlendMap.resize(65 * 65 * 4);
        for (int i = 0; i < 65 * 65; i++) {
            uint16 val = r.u16le();
            mBlendMap[i * 4 + 0] = static_cast<uint8>(((val >> 0) & 0xF) * 255 / 15);
            mBlendMap[i * 4 + 1] = static_cast<uint8>(((val >> 4) & 0xF) * 255 / 15);
            mBlendMap[i * 4 + 2] = static_cast<uint8>(((val >> 8) & 0xF) * 255 / 15);
            mBlendMap[i * 4 + 3] = static_cast<uint8>(((val >> 12) & 0xF) * 255 / 15);
        }
    }

    if ((mFlags & ChnkCellFlags::ColorMap) && r.can(8450)) {
        mColorMap.resize(65 * 65 * 4);
        for (int i = 0; i < 65 * 65; i++) {
            uint16 val = r.u16le();
            uint8 r5 = (val >> 0) & 0x1F;
            uint8 g6 = (val >> 5) & 0x3F;
            uint8 b5 = (val >> 11) & 0x1F;
            mColorMap[i * 4 + 0] = static_cast<uint8>((r5 * 255) / 31);
            mColorMap[i * 4 + 1] = static_cast<uint8>((g6 * 255) / 63);
            mColorMap[i * 4 + 2] = static_cast<uint8>((b5 * 255) / 31);
            mColorMap[i * 4 + 3] = 255;
        }
    }

    if ((mFlags & ChnkCellFlags::UnkMap) && r.can(8450)) {
        mUnknownMap.resize(65 * 65);
        for (int j = 0; j < 65 * 65; j++) {
            mUnknownMap[j] = r.u16le();
        }
    }

    if ((mFlags & ChnkCellFlags::Unk0x20) && r.can(4)) {
        mUnk0x20 = static_cast<int32>(r.u32le());
    }

    if ((mFlags & ChnkCellFlags::SkyIDs) && r.can(64)) {
        for (int corner = 0; corner < 4; corner++) {
            for (int skyIdx = 0; skyIdx < 4; skyIdx++) {
                mSkyCorners[corner].worldSkyIDs[skyIdx] = r.u32le();
            }
        }
    }

    if ((mFlags & ChnkCellFlags::SkyWeights) && r.can(16)) {
        for (int corner = 0; corner < 4; corner++) {
            for (int weightIdx = 0; weightIdx < 4; weightIdx++) {
                mSkyCorners[corner].worldSkyWeights[weightIdx] = r.u8();
            }
        }
    }

    if ((mFlags & ChnkCellFlags::ShadowMap) && r.can(4225)) {
        mShadowMap.resize(65 * 65);
        for (int j = 0; j < 65 * 65; j++) {
            mShadowMap[j] = r.u8();
        }
    }

    if ((mFlags & ChnkCellFlags::LoDHeightMap) && r.can(2178)) {
        mLodHeightMap.resize(33 * 33);
        for (int j = 0; j < 33 * 33; j++) {
            mLodHeightMap[j] = r.u16le();
        }
    }

    if ((mFlags & ChnkCellFlags::LoDHeightRange) && r.can(4)) {
        mLodHeightRange[0] = r.u16le();
        mLodHeightRange[1] = r.u16le();
    }

    if ((mFlags & ChnkCellFlags::Unk0x800) && r.can(578)) {
        r.skip(578);
    }

    if ((mFlags & ChnkCellFlags::Unk0x1000) && r.can(1)) {
        r.skip(1);
    }

    if ((mFlags & ChnkCellFlags::ColorMapDXT) && r.can(4624)) {
        mColorMapDXT.resize(4624);
        for (int j = 0; j < 4624; j++) {
            mColorMapDXT[j] = r.u8();
        }
    }

    if ((mFlags & ChnkCellFlags::UnkMap0DXT) && r.can(2312)) {
        r.skip(2312);
    }

    if ((mFlags & ChnkCellFlags::Unk0x8000) && r.can(8450)) {
        r.skip(8450);
    }

    if ((mFlags & ChnkCellFlags::ZoneBound) && r.can(4096)) {
        mZoneBounds.resize(64 * 64);
        for (int j = 0; j < 64 * 64; j++) {
            mZoneBounds[j] = r.u8();
        }
    }

    if ((mFlags & ChnkCellFlags::BlendMapDXT) && r.can(2312)) {
        mBlendMapDXT.resize(2312);
        for (int j = 0; j < 2312; j++) {
            mBlendMapDXT[j] = r.u8();
        }
    }

    if ((mFlags & ChnkCellFlags::UnkMap1DXT) && r.can(2312)) {
        mUnkMap1.resize(2312);
        for (int j = 0; j < 2312; j++) {
            mUnkMap1[j] = r.u8();
        }
    }

    if ((mFlags & ChnkCellFlags::UnkMap2DXT) && r.can(2312)) {
        r.skip(2312);
    }

    if ((mFlags & ChnkCellFlags::UnkMap3DXT) && r.can(2312)) {
        r.skip(2312);
    }

    if ((mFlags & ChnkCellFlags::Unk0x200000) && r.can(1)) {
        r.skip(1);
    }

    if ((mFlags & ChnkCellFlags::Unk0x400000) && r.can(16)) {
        r.skip(16);
    }

    if ((mFlags & ChnkCellFlags::Unk0x800000) && r.can(16900)) {
        r.skip(16900);
    }

    if ((mFlags & ChnkCellFlags::Unk0x1000000) && r.can(8)) {
        r.skip(8);
    }

    if ((mFlags & ChnkCellFlags::Unk0x2000000) && r.can(8450)) {
        r.skip(8450);
    }

    if ((mFlags & ChnkCellFlags::Unk0x4000000) && r.can(21316)) {
        r.skip(21316);
    }

    if ((mFlags & ChnkCellFlags::Unk0x8000000) && r.can(4096)) {
        r.skip(4096);
    }

    if ((mFlags & ChnkCellFlags::Zone) && r.can(16)) {
        for (int j = 0; j < 4; j++) {
            mZoneIds[j] = r.u32le();
        }
    }

    if ((mFlags & ChnkCellFlags::Unk0x20000000) && r.can(8450)) {
        r.skip(8450);
    }

    if ((mFlags & ChnkCellFlags::Unk0x40000000) && r.can(8450)) {
        r.skip(8450);
    }

    if ((mFlags & ChnkCellFlags::UnkMap4DXT) && r.can(2312)) {
        r.skip(2312);
    }

    while (r.can(8)) {
        uint32 chunkID = r.u32le();
        uint32 chunkSize = r.u32le();

        if (!r.can(chunkSize)) break;

        switch (chunkID) {
            case 0x504F5250:
            {
                uint32 propCount = chunkSize / 4;
                mProps.uniqueIDs.resize(propCount);
                for (uint32 i = 0; i < propCount; i++) {
                    mProps.uniqueIDs[i] = r.u32le();
                }
                break;
            }
            case 0x44727563:
            {
                mCurd.rawData.resize(chunkSize);
                for (uint32 i = 0; i < chunkSize; i++) {
                    mCurd.rawData[i] = r.u8();
                }
                break;
            }
            case 0x47744157:
            {
                if (chunkSize >= 4) {
                    uint32 waterCount = r.u32le();
                    if (waterCount > 0 && waterCount < 1000) {
                        mWaters.resize(waterCount);
                        uint32 remainingBytes = chunkSize - 4;
                        uint32 bytesPerWater = waterCount > 0 ? remainingBytes / waterCount : 0;
                        for (uint32 i = 0; i < waterCount && r.can(bytesPerWater); i++) {
                            mWaters[i].rawData.resize(bytesPerWater);
                            for (uint32 j = 0; j < bytesPerWater; j++) {
                                mWaters[i].rawData[j] = r.u8();
                            }
                        }
                        mHasWater = true;
                    }
                }
                break;
            }
            default:
                r.skip(chunkSize);
                break;
        }
    }

    if (mBlendMap.empty() && mBlendMapDXT.empty()) {
        mBlendMap.resize(65 * 65 * 4);
        for (int i = 0; i < 65 * 65; i++) {
            mBlendMap[i * 4 + 0] = 255;
            mBlendMap[i * 4 + 1] = 0;
            mBlendMap[i * 4 + 2] = 0;
            mBlendMap[i * 4 + 3] = 0;
        }
    }

    if (mColorMap.empty() && mColorMapDXT.empty()) {
        mColorMap.resize(65 * 65 * 4);
        for (int i = 0; i < 65 * 65; i++) {
            mColorMap[i * 4 + 0] = 128;
            mColorMap[i * 4 + 1] = 128;
            mColorMap[i * 4 + 2] = 128;
            mColorMap[i * 4 + 3] = 255;
        }
    }

    if (!hasHeightmap) {
        return;
    }

    float baseX = static_cast<float>(cellY) * 32.0f;
    float baseZ = static_cast<float>(cellX) * 32.0f;

    float totalHeight = 0.0f;
    uint32 validHeights = 0;

    mVertices.resize(17 * 17);

    for (int y = 0; y < 17; y++) {
        for (int x = 0; x < 17; x++) {
            uint16 h = heightmap[x * 19 + y] & 0x7FFF;
            float height = (static_cast<float>(h) / 8.0f) - 2048.0f;

            AreaVertex v{};
            v.x = baseX + static_cast<float>(x) * UnitSize;
            v.z = baseZ + static_cast<float>(y) * UnitSize;
            v.y = height;
            v.u = static_cast<float>(y) / 16.0f;
            v.v = static_cast<float>(x) / 16.0f;
            v.nx = 0.0f;
            v.ny = 1.0f;
            v.nz = 0.0f;
            v.tanx = 1.0f;
            v.tany = 0.0f;
            v.tanz = 0.0f;
            v.tanw = 1.0f;

            if (height > mMaxHeight) mMaxHeight = height;
            totalHeight += height;
            validHeights++;

            glm::vec3 pos(v.x, v.y, v.z);
            mMinBounds = glm::min(mMinBounds, pos);
            mMaxBounds = glm::max(mMaxBounds, pos);

            mVertices[y * 17 + x] = v;
        }
    }

    if (validHeights > 0) {
        mAverageHeight = totalHeight / static_cast<float>(validHeights);
    }

    calcNormals();

    if (indices.empty()) {
        indices.reserve(16 * 16 * 6);
        for (uint32 y = 0; y < 16; y++) {
            for (uint32 x = 0; x < 16; x++) {
                uint32 tl = y * 17 + x;
                uint32 tr = y * 17 + x + 1;
                uint32 bl = (y + 1) * 17 + x;
                uint32 br = (y + 1) * 17 + x + 1;

                indices.push_back(tl);
                indices.push_back(bl);
                indices.push_back(tr);

                indices.push_back(tr);
                indices.push_back(bl);
                indices.push_back(br);
            }
        }
    }

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
    if (mBlendMapTexture) glDeleteTextures(1, &mBlendMapTexture);
    if (mColorMapTextureGPU) glDeleteTextures(1, &mColorMapTextureGPU);
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
    if (mEBO) glDeleteBuffers(1, &mEBO);
}

void AreaChunkRender::render() {
    if (!mVAO) return;
    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AreaChunkRender::calcNormals() {
    for (int y = 0; y < 17; y++) {
        for (int x = 0; x < 17; x++) {
            glm::vec3 normal(0.0f, 0.0f, 0.0f);

            auto getPos = [&](int px, int py) -> glm::vec3 {
                px = std::clamp(px, 0, 16);
                py = std::clamp(py, 0, 16);
                auto& v = mVertices[py * 17 + px];
                return glm::vec3(v.x, v.y, v.z);
            };

            glm::vec3 left = getPos(x - 1, y);
            glm::vec3 right = getPos(x + 1, y);
            glm::vec3 up = getPos(x, y - 1);
            glm::vec3 down = getPos(x, y + 1);

            glm::vec3 dx = right - left;
            glm::vec3 dz = down - up;

            normal = glm::normalize(glm::cross(dz, dx));

            mVertices[y * 17 + x].nx = normal.x;
            mVertices[y * 17 + x].ny = normal.y;
            mVertices[y * 17 + x].nz = normal.z;
        }
    }
}

void AreaChunkRender::calcTangentBitangent() {
}

void AreaChunkRender::extendBuffer() {
}

void AreaChunkRender::geometryInit(uint32 program) {
    uniforms.colorTexture = glGetUniformLocation(program, "colorTexture");
    uniforms.alphaTexture = glGetUniformLocation(program, "alphaTexture");
    uniforms.hasColorMap = glGetUniformLocation(program, "hasColorMap");
    uniforms.texScale = glGetUniformLocation(program, "texScale");
    uniforms.camPosition = glGetUniformLocation(program, "camPosition");
    uniforms.model = glGetUniformLocation(program, "model");
    uniforms.highlightColor = glGetUniformLocation(program, "highlightColor");
    uniforms.baseColor = glGetUniformLocation(program, "baseColor");
    for (int i = 0; i < 4; ++i) {
        std::string t = "textures[" + std::to_string(i) + "]";
        std::string n = "normalTextures[" + std::to_string(i) + "]";
        uniforms.textures[i] = glGetUniformLocation(program, t.c_str());
        uniforms.normalTextures[i] = glGetUniformLocation(program, n.c_str());
    }
}