#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include "../Archive.h"
#include "../Utils/BinStream.h"
#include "Props.h"

typedef glm::mat4 Matrix;
typedef glm::vec3 Vector3;
typedef glm::vec4 Vector4;

#pragma pack(push, 1)
struct AreaVertex
{
    float x, y, z;
    float nx, ny, nz;
    float tanx, tany, tanz, tanw;
    float u, v;
};
#pragma pack(pop)

namespace ChnkCellFlags
{
    constexpr uint32 HeightMap       = 0x00000001;
    constexpr uint32 WorldLayerIDs   = 0x00000002;
    constexpr uint32 BlendMap        = 0x00000004;
    constexpr uint32 ColorMap        = 0x00000008;
    constexpr uint32 UnkMap          = 0x00000010;
    constexpr uint32 Unk0x20         = 0x00000020;
    constexpr uint32 SkyIDs          = 0x00000040;
    constexpr uint32 SkyWeights      = 0x00000080;
    constexpr uint32 ShadowMap       = 0x00000100;
    constexpr uint32 LoDHeightMap    = 0x00000200;
    constexpr uint32 LoDHeightRange  = 0x00000400;
    constexpr uint32 Unk0x800        = 0x00000800;
    constexpr uint32 Unk0x1000       = 0x00001000;
    constexpr uint32 ColorMapDXT     = 0x00002000;
    constexpr uint32 UnkMap0DXT      = 0x00004000;
    constexpr uint32 Unk0x8000       = 0x00008000;
    constexpr uint32 ZoneBound       = 0x00010000;
    constexpr uint32 BlendMapDXT     = 0x00020000;
    constexpr uint32 UnkMap1DXT      = 0x00040000;
    constexpr uint32 UnkMap2DXT      = 0x00080000;
    constexpr uint32 UnkMap3DXT      = 0x00100000;
    constexpr uint32 Unk0x200000     = 0x00200000;
    constexpr uint32 Unk0x400000     = 0x00400000;
    constexpr uint32 Unk0x800000     = 0x00800000;
    constexpr uint32 Unk0x1000000    = 0x01000000;
    constexpr uint32 Unk0x2000000    = 0x02000000;
    constexpr uint32 Unk0x4000000    = 0x04000000;
    constexpr uint32 Unk0x8000000    = 0x08000000;
    constexpr uint32 Zone            = 0x10000000;
    constexpr uint32 Unk0x20000000   = 0x20000000;
    constexpr uint32 Unk0x40000000   = 0x40000000;
    constexpr uint32 UnkMap4DXT      = 0x80000000;
}

namespace AreaChunkID
{
    constexpr uint32 CHNK = 0x43484E4B;
    constexpr uint32 PROp = 0x50524F70;
    constexpr uint32 CURT = 0x43555254;
    constexpr uint32 area = 0x61726561;
    constexpr uint32 AREA = 0x41524541;
}

constexpr float UnitSize = 2.0f;

struct SkyCorner
{
    uint32 worldSkyIDs[4] = {0};
    uint8 worldSkyWeights[4] = {0};
};

struct WaterData
{
    std::vector<uint8> rawData;
};

struct PropData
{
    std::vector<uint32> uniqueIDs;
};

struct CurdData
{
    std::vector<uint8> rawData;
};

struct ParsedChunk
{
    uint32 cellX = 0;
    uint32 cellY = 0;
    uint32 flags = 0;

    std::vector<AreaVertex> vertices;
    std::vector<uint8> blendMap;
    std::vector<uint8> blendMapDXT;
    std::vector<uint8> colorMap;
    std::vector<uint8> colorMapDXT;
    std::vector<uint16> unknownMap;
    int32 unk0x20 = 0;
    SkyCorner skyCorners[4];
    std::vector<uint8> shadowMap;
    std::vector<uint16> lodHeightMap;
    uint16 lodHeightRange[2] = {0};
    std::vector<uint8> zoneBounds;
    std::vector<uint8> unkMap1;
    uint32 worldLayerIDs[4] = {0};
    uint32 zoneIds[4] = {0};

    PropData props;
    CurdData curd;
    std::vector<WaterData> waters;
    bool hasWater = false;

    float maxHeight = -100000.0f;
    float avgHeight = 0.0f;
    glm::vec3 minBounds{std::numeric_limits<float>::max()};
    glm::vec3 maxBounds{std::numeric_limits<float>::lowest()};

    bool valid = false;
};

class AreaChunkRender
{
public:
    struct Uniforms
    {
        uint32 colorTexture = 0;
        uint32 alphaTexture = 0;
        uint32 hasColorMap = 0;
        uint32 textures[4]{};
        uint32 normalTextures[4]{};
        uint32 texScale = 0;
        uint32 camPosition = 0;
        uint32 model = 0;
        uint32 highlightColor = 0;
        uint32 baseColor = 0;
    };

private:
    static std::vector<uint32> indices;
    static Uniforms uniforms;

    std::vector<uint8> mChunkData;
    unsigned int mVAO = 0;
    unsigned int mVBO = 0;
    unsigned int mEBO = 0;
    int mIndexCount = 0;

    float mMaxHeight = -100000.0f;
    float mAverageHeight = 0.0f;

    glm::vec3 mMinBounds;
    glm::vec3 mMaxBounds;

    std::vector<AreaVertex> mVertices;

    uint32 mSplatTexture = 0;
    uint32 mColorMapTexture = 0;
    std::vector<uint32> mLayerTextures;

    uint32 mFlags = 0;
    uint32 mZoneIds[4] = {0};

    uint32 mWorldLayerIDs[4] = {0};
    std::vector<uint8> mBlendMap;
    std::vector<uint8> mColorMap;
    std::vector<uint16> mUnknownMap;
    int32 mUnk0x20 = 0;
    SkyCorner mSkyCorners[4];
    std::vector<uint8> mShadowMap;
    std::vector<uint16> mLodHeightMap;
    uint16 mLodHeightRange[2] = {0};
    std::vector<uint8> mZoneBounds;
    std::vector<uint8> mBlendMapDXT;
    std::vector<uint8> mColorMapDXT;
    std::vector<uint8> mUnkMap1;

    PropData mProps;
    CurdData mCurd;
    std::vector<WaterData> mWaters;
    bool mHasWater = false;

    unsigned int mBlendMapTexture = 0;
    unsigned int mColorMapTextureGPU = 0;
    unsigned int mLayerDiffuse[4] = {0, 0, 0, 0};
    unsigned int mLayerNormal[4] = {0, 0, 0, 0};
    float mLayerScale[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    bool mTexturesLoaded = false;

    void calcNormals();
    void calcTangentBitangent();
    void extendBuffer();
    void uploadGPU();

public:
    AreaChunkRender(const std::vector<uint8>& cellData, uint32 cellX, uint32 cellY, ArchivePtr archive);
    AreaChunkRender(ParsedChunk&& parsed, ArchivePtr archive);
    ~AreaChunkRender();

    [[nodiscard]] float getMaxHeight() const { return mMaxHeight; }
    [[nodiscard]] float getAverageHeight() const { return mAverageHeight; }
    [[nodiscard]] glm::vec3 getMinBounds() const { return mMinBounds; }
    [[nodiscard]] glm::vec3 getMaxBounds() const { return mMaxBounds; }
    [[nodiscard]] uint32 getFlags() const { return mFlags; }

    [[nodiscard]] bool hasHeightmap() const { return (mFlags & ChnkCellFlags::HeightMap) != 0; }
    [[nodiscard]] bool hasZoneBounds() const { return (mFlags & ChnkCellFlags::ZoneBound) != 0; }
    [[nodiscard]] bool hasZoneIDs() const { return (mFlags & ChnkCellFlags::Zone) != 0; }
    [[nodiscard]] bool hasWorldLayerIDs() const { return (mFlags & ChnkCellFlags::WorldLayerIDs) != 0; }
    [[nodiscard]] bool hasBlendMap() const { return (mFlags & ChnkCellFlags::BlendMap) != 0 || (mFlags & ChnkCellFlags::BlendMapDXT) != 0; }
    [[nodiscard]] bool hasColorMap() const { return (mFlags & ChnkCellFlags::ColorMap) != 0 || (mFlags & ChnkCellFlags::ColorMapDXT) != 0; }
    [[nodiscard]] bool hasSkyData() const { return (mFlags & ChnkCellFlags::SkyIDs) != 0; }
    [[nodiscard]] bool hasLodHeightMap() const { return (mFlags & ChnkCellFlags::LoDHeightMap) != 0; }
    [[nodiscard]] bool hasShadowMap() const { return (mFlags & ChnkCellFlags::ShadowMap) != 0; }
    [[nodiscard]] bool hasWater() const { return mHasWater; }

    [[nodiscard]] const uint32* getWorldLayerIDs() const { return mWorldLayerIDs; }
    [[nodiscard]] const std::vector<uint8>& getBlendMap() const { return mBlendMap; }
    [[nodiscard]] const std::vector<uint8>& getBlendMapDXT() const { return mBlendMapDXT; }
    [[nodiscard]] const std::vector<uint8>& getColorMap() const { return mColorMap; }
    [[nodiscard]] const std::vector<uint8>& getColorMapDXT() const { return mColorMapDXT; }
    [[nodiscard]] const SkyCorner* getSkyCorners() const { return mSkyCorners; }
    [[nodiscard]] const std::vector<uint16>& getLodHeightMap() const { return mLodHeightMap; }
    [[nodiscard]] const uint16* getLodHeightRange() const { return mLodHeightRange; }
    [[nodiscard]] const std::vector<uint8>& getShadowMap() const { return mShadowMap; }
    [[nodiscard]] const PropData& getProps() const { return mProps; }
    [[nodiscard]] const std::vector<WaterData>& getWaters() const { return mWaters; }

    [[nodiscard]] const std::vector<AreaVertex>& getVertices() const { return mVertices; }
    [[nodiscard]] static const std::vector<uint32>& getIndices() { return indices; }
    [[nodiscard]] const float* getLayerScales() const { return mLayerScale; }

    [[nodiscard]] bool isFullyInitialized() const { return mVAO != 0; }

    void loadTextures(const ArchivePtr& archive);
    void bindTextures(unsigned int program) const;
    void render();

    static void geometryInit(uint32 program);
    static const Uniforms& getUniforms();
    static ParsedChunk parseChunkData(const std::vector<uint8>& cellData, uint32 cellX, uint32 cellY);
};

typedef std::shared_ptr<AreaChunkRender> AreaChunkRenderPtr;

struct ParsedArea
{
    FileEntryPtr file;
    std::wstring path;
    int tileX = 0;
    int tileY = 0;
    std::vector<ParsedChunk> chunks;
    std::vector<Prop> props;
    std::vector<CurtData> curts;
    float maxHeight = -100000.0f;
    float avgHeight = 0.0f;
    glm::vec3 minBounds{std::numeric_limits<float>::max()};
    glm::vec3 maxBounds{std::numeric_limits<float>::lowest()};
    bool valid = false;
};

class AreaFile
{
    ArchivePtr mArchive;
    FileEntryPtr mFile;
    std::shared_ptr<BinStream> mStream;
    std::vector<uint8> mContent;
    std::vector<AreaChunkRenderPtr> mChunks;
    std::wstring mPath;
    float mMaxHeight = -100000.0f;
    float mAverageHeight = 0.0f;

    glm::vec3 mMinBounds;
    glm::vec3 mMaxBounds;
    float mGlobalRotation = 0.0f;

    glm::vec4 mBaseColor = glm::vec4(1.0f);

    int mTileX = 0;
    int mTileY = 0;

    glm::vec3 mWorldOffset = glm::vec3(0.0f);

    unsigned int mTextureID = 0;
    bool mHasTexture = false;

    std::vector<Prop> mProps;
    std::unordered_map<uint32_t, size_t> mPropLookup;
    std::vector<CurtData> mCurts;
    std::unordered_map<uint32_t, std::shared_ptr<M3Render>> mPropRenderers;

    void parseTileXYFromFilename();
    bool loadTexture();
    void calculateWorldOffset();

public:
    AreaFile(ArchivePtr archive, FileEntryPtr file);
    ~AreaFile();
    bool load();
    bool loadFromParsed(ParsedArea&& parsed);

    void setTileXY(int tx, int ty) { mTileX = tx; mTileY = ty; calculateWorldOffset(); }
    [[nodiscard]] int getTileX() const { return mTileX; }
    [[nodiscard]] int getTileY() const { return mTileY; }

    void render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram, const AreaChunkRenderPtr& selectedChunk);
    void renderProps(const Matrix& matView, const Matrix& matProj);
    bool loadProp(uint32_t uniqueID);
    void loadAllProps();
    void loadAllPropsWithProgress(std::function<void(size_t, size_t)> progressCallback);
    void loadAllPropsAsync();
    void loadPropsInView(const glm::vec3& cameraPos, float radius);
    void updatePropLoading();

    [[nodiscard]] float getMaxHeight() const { return mMaxHeight; }
    [[nodiscard]] float getAverageHeight() const { return mAverageHeight; }

    [[nodiscard]] glm::vec3 getMinBounds() const { return mMinBounds; }
    [[nodiscard]] glm::vec3 getMaxBounds() const { return mMaxBounds; }

    [[nodiscard]] glm::vec3 getWorldMinBounds() const { return mMinBounds + mWorldOffset; }
    [[nodiscard]] glm::vec3 getWorldMaxBounds() const { return mMaxBounds + mWorldOffset; }
    [[nodiscard]] glm::vec3 getWorldOffset() const { return mWorldOffset; }

    [[nodiscard]] ArchivePtr getArchive() const { return mArchive; }
    [[nodiscard]] const std::wstring& getPath() const { return mPath; }
    [[nodiscard]] FileEntryPtr getFile() const { return mFile; }

    static constexpr int WORLD_GRID_ORIGIN = 64;
    static const float GRID_SIZE;

    void rotate90() {
        mGlobalRotation += 90.0f;
        if (mGlobalRotation >= 360.0f) mGlobalRotation -= 360.0f;
    }
    void rotate90CCW() {
        mGlobalRotation -= 90.0f;
        if (mGlobalRotation < 0.0f) mGlobalRotation += 360.0f;
    }
    [[nodiscard]] float getRotation() const { return mGlobalRotation; }

    void printRotationDebug() const;

    [[nodiscard]] const std::vector<AreaChunkRenderPtr>& getChunks() const { return mChunks; }
    [[nodiscard]] const std::vector<Prop>& getProps() const { return mProps; }
    [[nodiscard]] size_t getPropCount() const { return mProps.size(); }
    [[nodiscard]] const Prop* getPropByID(uint32_t uniqueID) const;
    [[nodiscard]] size_t getLoadedPropCount() const;
    [[nodiscard]] const std::vector<CurtData>& getCurts() const { return mCurts; }

    static const float UnitSize;
    static ParsedArea parseAreaFile(const ArchivePtr& archive, const FileEntryPtr& file);
};

typedef std::shared_ptr<AreaFile> AreaFilePtr;

void ResetAreaReferencePosition();