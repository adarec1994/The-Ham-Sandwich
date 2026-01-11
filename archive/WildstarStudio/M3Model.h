#pragma once

#include "Archive.h"
#include "IOManager.h"
#include "BinStream.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "InputGeometry.h"
#include "TextureInput.h"
#include "Pipeline.h"
#include "Texture.h"

#pragma pack(push, 1)

struct M3ModelVertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

#pragma pack(pop)

class M3Model
{
#pragma pack(push, 1)

	struct M3Header
	{
		uint8 various1[0x1A0];
		uint64 nTextures;
		uint64 ofsTextures;
		uint8 various5[0x20];
		uint64 nMaterials;
		uint64 ofsMaterials;
		uint8 various2[0x20];
		uint64 nVertices;
		uint64 ofsVertices;
		uint64 nIndices; // not actually indices for the geometry
		uint64 ofsIndices; // same here
		uint64 nSubMeshes;
		uint64 ofsSubMeshes;
		uint64 various3[7];
		uint64 nViews;
		uint64 ofsViews;
		uint8 various4[0x3E8];
	};

	struct M3Texture
	{
		uint32 unk[4];
		uint64 lenName;
		union {
			uint64 ofsName;
			wchar_t* name;
		};
	};

	struct M3Vertex
	{
		float x, y, z;
		uint8 indices[4];
		int8 normals[4];
		int8 tangents[4];
		uint32 unk[4];
		uint16 s, t, u, v;
	};

	struct M3SubMesh
	{
		uint32 startIndex;
		uint32 startVertex;
		uint32 nIndices;
		uint32 nVertices;
		uint32 unk1;
		uint16 unk2;
		uint16 material;
		uint32 unk8;
		uint32 color2;
		uint32 unk3, unk4, unk5, unk6;
		uint32 color3;
		uint32 color4;
		uint32 unk7; // could be a color or even a float
		uint8 pad[36];
	};

	struct M3Skin
	{
		uint64 sizeOfStruct;
		uint64 nVertexLookup;
		uint64 ofsVertexLookup;
		uint64 nUnk1;
		uint64 ofsUnk1;
		uint64 nIndexLookup;
		uint64 ofsIndexLookup;
		uint64 nUnk2; // somehow related to the submeshes
		uint64 ofsUnk2;
	};

	struct M3Material
	{
		uint32 unk1[8];
		uint64 nTextures;
		uint64 ofsTextures;
	};

	struct Vector
	{
		float x, y, z;

		Vector normalized() {
			float len = length();
			return { x / len, y / len, z / len };
		}

		float length() {
			return sqrtf(x * x + y * y + z * z);
		}

		static Vector mul(const Vector& a, float b) {
			return { a.x * b, a.y * b, a.z * b };
		}

		static Vector sub(const Vector& a, const Vector& b) {
			return { a.x - b.x, a.y - b.y, a.z - b.z };
		}

		static Vector add(const Vector& a, const Vector& b) {
			return { a.x + b.x, a.y + b.y, a.z + b.z };
		}

		static Vector cross(const Vector& a, const Vector& b) {
			return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
		}
	};

	struct Triangle
	{
		Vector vertices[3];
		Vector normal;
		Vector center;
		Vector normals[3];

		uint32 indices[3];

		int32 getVertexIndex(uint32 index) {
			for (uint32 i = 0; i < 3; ++i) {
				if (indices[i] == index) {
					return i;
				}
			}

			return -1;
		}
	};

#pragma pack(pop)

	M3Header mHeader;
	std::vector<M3Vertex> mVertices;
	std::vector<Vector> mNormals;
	GLuint mDisplayList;
	std::unique_ptr<BinStream> mStream;
	std::vector<uint8> mContent;
	std::wstring mModelName;
	std::vector<std::vector<Triangle>> mMeshTriangles;
	std::vector<M3SubMesh> mMeshes;
	std::vector<M3Skin> mSkins;
	std::vector<M3ModelVertex>  mModelVertices;
	std::wstring mPath;
	VertexBufferPtr mVertexBuffer;
	std::vector<IndexBufferPtr> mMeshIndices;
	std::vector<M3Material> mMaterials;
	std::vector<TexturePtr> mTextures;

	float mMaxDistance;
	float mHeight;

	void computeNormals();

public:
	M3Model(const std::wstring& name, FileEntryPtr file);

	void exportAsObj(bool sync = false);

	void load();
	void loadForExport();
	void render(InputGeometryPtr geom, TextureInputPtr tex);

	float getMaxDistance() const { return mMaxDistance; }
	float getMaxHeight() const { return mHeight; }
};

SHARED_TYPE(M3Model);