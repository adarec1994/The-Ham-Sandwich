#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../Archive.h"
#include "../Utils/BinStream.h"
#include "DataTable.h"

typedef glm::mat4 Matrix;
typedef glm::vec3 Vector3;
typedef glm::vec4 Vector4;

#pragma pack(push, 1)
struct WorldLayerEntry
{
    uint32 id;
    wchar_t* description;
    float heightScale;
    float heightOffset;
    float parallaxScale;
    float parallaxOffset;
    float metersPerTexture;
    wchar_t* colorMapPath;
    wchar_t* normalMapPath;
    uint32 averageColor;
    uint32 projection;
    uint32 materialType;
    uint32 worldClutterId00;
    uint32 worldClutterId01;
    uint32 worldClutterId02;
    uint32 worldClutterId03;
    float specularPower;
    float emissiveGlow;
    float scrollSpeed00;
    float scrollSpeed01;
};

struct AreaVertex
{
    float x, y, z;
    float nx, ny, nz;
    float tanx, tany, tanz, tanw;
    float u, v;
};
#pragma pack(pop)

// ChnkCell flags - from NexusForever ChnkCellFlags.cs
// These are the FLAG BITS, not sequential values!
namespace ChnkCellFlags
{
    constexpr uint32 HeightMap  = 0x00000001;  // Bit 0
    constexpr uint32 ZoneBound  = 0x00010000;  // Bit 16
    constexpr uint32 Zone       = 0x10000000;  // Bit 28

    // Additional flags we care about for rendering (from dataSize array positions)
    constexpr uint32 WorldLayerIDs  = 0x00000002;  // Bit 1
    constexpr uint32 BlendMap       = 0x00000004;  // Bit 2
    constexpr uint32 ColorMap       = 0x00000008;  // Bit 3
    constexpr uint32 ColorMapDXT    = 0x00002000;  // Bit 13
    constexpr uint32 BlendMapDXT    = 0x00020000;  // Bit 17
}

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
    std::vector<AreaVertex> mFullVertices;

    uint32 mSplatTexture = 0;
    uint32 mColorMapTexture = 0;
    std::vector<uint32> mLayerTextures;

    uint32 mFlags = 0;
    Vector4 mTexScales = Vector4(1.0f);

    uint32 mWorldLayerIDs[4] = {0};
    uint32 mZoneIds[4] = {0};

    void calcNormals();
    void calcTangentBitangent();
    void extendBuffer();

public:
    // Constructor takes raw cell data (flags are inside the data)
    AreaChunkRender(const std::vector<uint8>& cellData, float baseX, float baseZ, ArchivePtr archive);
    ~AreaChunkRender();

    [[nodiscard]] float getMaxHeight() const { return mMaxHeight; }
    [[nodiscard]] float getAverageHeight() const { return mAverageHeight; }
    [[nodiscard]] glm::vec3 getMinBounds() const { return mMinBounds; }
    [[nodiscard]] glm::vec3 getMaxBounds() const { return mMaxBounds; }
    [[nodiscard]] uint32 getFlags() const { return mFlags; }

    // Flag checks using ChnkCellFlags bit positions
    [[nodiscard]] bool hasHeightmap() const { return (mFlags & ChnkCellFlags::HeightMap) != 0; }
    [[nodiscard]] bool hasWorldLayerIDs() const { return (mFlags & ChnkCellFlags::WorldLayerIDs) != 0; }
    [[nodiscard]] bool hasBlendMap() const { return (mFlags & ChnkCellFlags::BlendMap) != 0; }
    [[nodiscard]] bool hasColorMap() const { return (mFlags & ChnkCellFlags::ColorMap) != 0; }
    [[nodiscard]] bool hasColorMapDXT() const { return (mFlags & ChnkCellFlags::ColorMapDXT) != 0; }
    [[nodiscard]] bool hasZoneBounds() const { return (mFlags & ChnkCellFlags::ZoneBound) != 0; }
    [[nodiscard]] bool hasBlendMapDXT() const { return (mFlags & ChnkCellFlags::BlendMapDXT) != 0; }
    [[nodiscard]] bool hasZoneIDs() const { return (mFlags & ChnkCellFlags::Zone) != 0; }

    // Returns true if this chunk was fully initialized (has geometry ready to render)
    [[nodiscard]] bool isFullyInitialized() const { return mVAO != 0; }

    void render();

    static void geometryInit(uint32 program);
    static const Uniforms& getUniforms();
};

typedef std::shared_ptr<AreaChunkRender> AreaChunkRenderPtr;

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

    void parseTileXYFromFilename();
    bool loadTexture();
    void calculateWorldOffset();

public:
    AreaFile(ArchivePtr archive, FileEntryPtr file);
    ~AreaFile();
    bool load();

    void setTileXY(int tx, int ty) { mTileX = tx; mTileY = ty; calculateWorldOffset(); }
    [[nodiscard]] int getTileX() const { return mTileX; }
    [[nodiscard]] int getTileY() const { return mTileY; }

    void render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram, const AreaChunkRenderPtr& selectedChunk);

    [[nodiscard]] float getMaxHeight() const { return mMaxHeight; }
    [[nodiscard]] float getAverageHeight() const { return mAverageHeight; }

    [[nodiscard]] glm::vec3 getMinBounds() const { return mMinBounds; }
    [[nodiscard]] glm::vec3 getMaxBounds() const { return mMaxBounds; }

    [[nodiscard]] glm::vec3 getWorldMinBounds() const { return mMinBounds + mWorldOffset; }
    [[nodiscard]] glm::vec3 getWorldMaxBounds() const { return mMaxBounds + mWorldOffset; }
    [[nodiscard]] glm::vec3 getWorldOffset() const { return mWorldOffset; }

    static constexpr int WORLD_GRID_ORIGIN = 64;
    static const float GRID_SIZE;

    void rotate90() { mGlobalRotation += 90.0f; }
    [[nodiscard]] float getRotation() const { return mGlobalRotation; }

    [[nodiscard]] const std::vector<AreaChunkRenderPtr>& getChunks() const { return mChunks; }

    static const float UnitSize;
};

typedef std::shared_ptr<AreaFile> AreaFilePtr;

// Call this when clearing loaded areas to reset reference tile position
void ResetAreaReferencePosition();