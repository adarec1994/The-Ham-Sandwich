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
    float x, y, z;       // Position
    float nx, ny, nz;    // Normal
    float tanx, tany, tanz, tanw;  // Tangent
    float u, v;          // Texture coordinates
};
#pragma pack(pop)

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

    std::vector<uint32> mBlendValues;
    uint32 mFlags = 0;
    Vector4 mTexScales = Vector4(1.0f);

    uint32 mZoneIds[4] = {0};

    void calcNormals();
    void calcTangentBitangent();
    void extendBuffer();

public:
    AreaChunkRender(uint32 flags, const std::vector<uint8>& payload, float baseX, float baseZ, ArchivePtr archive);
    ~AreaChunkRender();

    float getMaxHeight() const { return mMaxHeight; }
    float getAverageHeight() const { return mAverageHeight; }
    glm::vec3 getMinBounds() const { return mMinBounds; }
    glm::vec3 getMaxBounds() const { return mMaxBounds; }
    uint32 getFlags() const { return mFlags; }

    bool hasHeightmap() const { return (mFlags & 1) != 0; }
    bool hasTextureIds() const { return (mFlags & 2) != 0; }
    bool hasBlendValues() const { return (mFlags & 4) != 0; }
    bool hasColorMap() const { return (mFlags & 8) != 0; }
    bool hasUnk1() const { return (mFlags & 0x40) != 0 || (mFlags & 0x80) != 0; }
    bool hasShadowMap() const { return (mFlags & 0x100) != 0; }

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

    // World offset based on tile coordinates
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
    int getTileX() const { return mTileX; }
    int getTileY() const { return mTileY; }

    void render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram, AreaChunkRenderPtr selectedChunk);

    float getMaxHeight() const { return mMaxHeight; }
    float getAverageHeight() const { return mAverageHeight; }

    // Local bounds (within area file coordinate space)
    glm::vec3 getMinBounds() const { return mMinBounds; }
    glm::vec3 getMaxBounds() const { return mMaxBounds; }

    // World bounds (accounting for tile position)
    glm::vec3 getWorldMinBounds() const { return mMinBounds + mWorldOffset; }
    glm::vec3 getWorldMaxBounds() const { return mMaxBounds + mWorldOffset; }
    glm::vec3 getWorldOffset() const { return mWorldOffset; }

    // World grid constants
    static const int WORLD_GRID_ORIGIN = 64;
    static const float GRID_SIZE;

    void rotate90() { mGlobalRotation += 90.0f; }
    float getRotation() const { return mGlobalRotation; }

    const std::vector<AreaChunkRenderPtr>& getChunks() const { return mChunks; }

    // Unit size: distance between adjacent vertices (2 units)
    static const float UnitSize;
};

typedef std::shared_ptr<AreaFile> AreaFilePtr;