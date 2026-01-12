#pragma once

#include "IOManager.h"
#include "BinStream.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "InputGeometry.h"
#include "Pipeline.h"
#include "TextureInput.h"

#pragma pack(push, 1)

struct I3SectorHeader
{
	uint32 magic;
	uint32 version;
	uint64 various[3];
	uint64 lenSectorName;
	uint64 ofsSectorName;
	uint64 unk1;
	uint64 nVertices;
	uint64 ofsVertices;
	uint64 nVertexInfo;
	uint64 ofsVertexInfo;
	uint64 nIndices;
	uint64 ofsIndices;
	uint64 uUnk1;
	uint64 ofsUnk1;
	uint64 nSubMeshes;
	uint64 ofsSubMeshes;
	uint64 nUnk2;
	uint64 ofsUnk2;
	uint64 nUnk3;
	uint64 ofsUnk3;
	uint64 nUnk4;
	uint64 ofsUnk4;
	uint64 nUnk5;
	uint64 ofsUnk5;
	uint64 nUnk6;
	uint64 ofsUnk6;
	uint64 nUnk7;
	uint64 ofsUnk7;
	uint64 nUnk8;
	uint64 ofsUnk8;
	uint64 nUnk9;
	uint64 ofsUnk9;
	uint64 nUnk10;
	uint64 ofsUnk10;
	uint64 pad1;
	float bboxMin[3];
	uint64 pad2;
	float bboxMax[3];
	uint64 nTexRelated1;
	uint64 ofsTexRelated1;
	uint64 nUnk11;
	uint64 ofsUnk11;
	uint64 unk12[4];
	uint64 nPasses;
	uint64 ofsPasses;
	uint64 nTexLookup;
	uint64 ofsTexLookup;
}; // sizeof(I3SectorHeader) == 0x1B0

struct TexRelated1
{
	uint64 someIndex;
	uint64 lenTextureName;
	uint64 ofsTextureName;
};

struct ModelPass
{
	uint64 unk1;
	uint64 lenPassName;
	uint64 ofsPassName;
	uint32 passIndex;
	uint32 baseLookup;
};

struct I3Vertex
{
	float x, y, z;
};

struct I3SubMesh
{
	uint32 passIndex;
	uint32 startIndex;
	uint32 endIndex; // inclusive
	uint32 startVertex;
	uint32 endVertex; // inclusive
	uint32 unk2;
	uint32 unk3;
};

struct I3ModelVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

class I3Model;

#pragma pack(pop)

class I3Sector
{
	FileEntryPtr mFile;
	std::wstring mSectorName;
	std::vector<uint8> mContent;
	std::unique_ptr<BinStream> mStream;
	std::vector<I3Vertex> mVertices;
	std::vector<I3SubMesh> mSubMeshes;
	std::vector<uint32> mIndices;
	std::vector<I3Vertex> mNormals;
	VertexBufferPtr mVertexBuffer;
	IndexBufferPtr mIndexBuffer;
	std::weak_ptr<I3Model> mParent;
	GLuint mDisplayList;
	float mHeight;
	float mMinPos;
	float mMaxDistance;
	std::vector<uint16> mPassTextureLookup;

	template<typename T>
	void read(std::vector<T>& dst, uint64 nElems, uint64 ofs) {
		mStream->seek(ofs + 0x1B0);
		dst.resize((uint32) nElems);
		mStream->read(dst.data(), sizeof(T) * nElems);
	}

	float getFixedPoint(int32 val) {
		float maxNormal = powf(2, 23);
		return (val - maxNormal) / maxNormal;
	}

	float getFixedPoint32(uint32 val) {
		return ((float) val / 0xFFFFFFFF);
	}

	void computeNormals();
public:
	I3Sector(const std::wstring& sectorName, const std::wstring& fileName, std::weak_ptr<I3Model> parent);

	float getMaxDistance() const { return mMaxDistance; }
	float getHeight() const { return mHeight; }
	float getMinPos() const { return mMinPos; }

	std::wstring getName() const { return mSectorName; }

	void load();

	void render(InputGeometryPtr geom, TextureInputPtr texInput);
};

SHARED_TYPE(I3Sector);