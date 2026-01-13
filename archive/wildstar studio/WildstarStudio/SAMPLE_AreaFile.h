#pragma once

#include "IOManager.h"
#include "BinStream.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Program.h"
#include "Matrix.h"
#include "Texture.h"

#pragma pack(push, 1)

/*struct AreaChunk
{
	uint32 flags;
	uint16 heightMap[19][19];
	uint32 textureLayers[4];
	uint16 blend[65][65];
	uint16 colorMap[65][65];
	uint16 unk1[40];
	uint8 shadowMap[65][65];
};*/

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

class AreaChunkRender
{
	struct Uniforms
	{
		uint32 colorTexture = 0;
		uint32 alphaTexture = 0;
		uint32 hasColorMap = 0;
		uint32 textures[4];
		uint32 normalTextures[4];
		uint32 texScale = 0;
		uint32 camPosition = 0;
	};

	static std::vector<uint32> indices;
	static Uniforms uniforms;

	std::vector<uint8> mChunkData;
	VertexBufferPtr mVertexBuffer;
	IndexBufferPtr mIndexBuffer;
	float mMaxHeight = -FLT_MAX;
	float mAverageHeight = 0.0f;
	std::vector<AreaVertex> mVertices;
	std::vector<AreaVertex> mFullVertices;
	std::vector<TexturePtr> mTextures;
	std::vector<TexturePtr> mNormalTextures;
	TexturePtr mBlendTexture;
	TexturePtr mBlendTexture2;
	std::vector<uint32> mBlendValues;
	uint32 mFlags;
	Vector4 mTexScales;

	void calcNormals();
	void calcTangentBitangent();
	void extendBuffer();

public:
	AreaChunkRender(uint32 flags, const std::vector<uint8>& chunkData, float baseX, float baseY, bool objOnly = false);

	float getMaxHeight() const { return mMaxHeight; }
	float getAverageHeight() const { return mAverageHeight; }

	void exportToObj(std::ofstream& os, uint32& baseIndex);

	std::vector<uint8>& getData() { return mChunkData; }

	bool hasHeightmap() const { return (mFlags & 1) != 0; }
	bool hasTextureIds() const { return (mFlags & 2) != 0; }
	bool hasBlendValues() const { return (mFlags & 4) != 0; }
	bool hasColorMap() const { return (mFlags & 8) != 0; }
	bool hasUnk1() const { return (mFlags & 0x40) != 0 || (mFlags & 0x80) != 0; }
	bool hasShadowMap() const { return (mFlags & 0x100) != 0; }
	bool hasUnk2() const { return (mFlags & 0x10000) != 0; }
	bool hasUnk3() const { return (mFlags & 0x200000) != 0; }
	bool hasUnk4() const { return (mFlags & 0x400000) != 0; }
	bool hasUnk5() const { return (mFlags & 0x4000000) != 0; }
	bool hasUnk6() const { return (mFlags & 0x8000000) != 0; }
	bool hasUnk7() const { return (mFlags & 0x10000000) != 0; }
	bool hasUnk8() const { return (mFlags & 0x20000000) != 0; }

	std::vector<uint8> getColorMap() const;
	std::vector<uint8> getBlendData() const;

	TexturePtr getTexture(uint32 index) const { return mTextures[index]; }
	uint32 getNumTextures() const { return mTextures.size(); }
	uint32 getUnk7(uint32 index) const;
	uint32 getTextureId(uint32 index) const;

	uint32 getFlags() { return mFlags; }

	void render();

	static void geometryInit(ProgramPtr program);
};

SHARED_TYPE(AreaChunkRender);

class AreaFile
{
	static uint32 gUniformView, gUniformProj, gUniformWorldPos;

	FileEntryPtr mFile;
	std::shared_ptr<BinStream> mStream;
	std::vector<uint8> mContent;
	std::vector<AreaChunkRenderPtr> mChunks;
	std::wstring mPath;
	std::wstring mModelName;
	float mMaxHeight = -FLT_MAX;
	float mAverageHeight = 0.0f;

	void initGeometry();

public:
	AreaFile(const std::wstring& name, FileEntryPtr file);
	bool load();
	bool loadForExport();

	void render(const Matrix& matView, const Matrix& matProj);

	float getMaxHeight() const { return mMaxHeight; }
	float getAverageHeight() const { return mAverageHeight; }
	void exportToObj(bool sync = false);

	std::vector<AreaChunkRenderPtr>& getChunks() { return mChunks; }

	static const float UnitSize;
};

SHARED_TYPE(AreaFile);