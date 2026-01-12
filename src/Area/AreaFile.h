#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <fstream>
#include "../Archive.h"
#include "../Utils/BinStream.h"

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

class AreaChunkRender
{
    struct Uniforms
    {
       uint32 colorTexture = 0;
       uint32 alphaTexture = 0;
       uint32 hasColorMap = 0;
       uint32 textures[4]{};
       uint32 normalTextures[4]{};
       uint32 texScale = 0;
       uint32 camPosition = 0;
    };

    static std::vector<uint32> indices;
    static Uniforms uniforms;
    static const int DataSizes[32];

    std::vector<uint8> mChunkData;
    unsigned int mVAO = 0;
    unsigned int mVBO = 0;
    unsigned int mEBO = 0;
    int mIndexCount = 0;

    float mMaxHeight = -100000.0f;
    float mAverageHeight = 0.0f;
    std::vector<AreaVertex> mVertices;
    std::vector<AreaVertex> mFullVertices;

    std::vector<uint32> mTextures;
    std::vector<uint32> mNormalTextures;
    uint32 mBlendTexture = 0;
    uint32 mBlendTexture2 = 0;

    std::vector<uint32> mBlendValues;
    uint32 mFlags = 0;
    Vector4 mTexScales = Vector4(1.0f);

    uint32 mZoneIds[4] = {0};

    void calcNormals();
    void calcTangentBitangent();
    void extendBuffer();

public:
    AreaChunkRender(uint32 flags, const std::vector<uint8>& payload, float baseX, float baseY, bool swapped);
    ~AreaChunkRender();

    float getMaxHeight() const { return mMaxHeight; }
    float getAverageHeight() const { return mAverageHeight; }

    bool hasHeightmap() const { return (mFlags & 1) != 0; }
    void render();

    static void geometryInit(uint32 program);
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

    int mTileX = 0;
    int mTileY = 0;

    unsigned int mTextureID = 0;
    bool mHasTexture = false;

    static bool sOriginSet;
    static int  sOriginTileX;
    static int  sOriginTileY;

    void parseTileXYFromFilename();
    bool loadTexture();

public:
    AreaFile(ArchivePtr archive, FileEntryPtr file);
    ~AreaFile();
    bool load();

    void setTileXY(int tx, int ty) { mTileX = tx; mTileY = ty; }

    void render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram);

    float getMaxHeight() const { return mMaxHeight; }
    float getAverageHeight() const { return mAverageHeight; }

    static const float UnitSize;
};

typedef std::shared_ptr<AreaFile> AreaFilePtr;