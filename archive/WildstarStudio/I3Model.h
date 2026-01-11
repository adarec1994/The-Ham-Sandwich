#pragma once

#include "IOManager.h"
#include "BinStream.h"
#include "I3Sector.h"
#include "Texture.h"

#pragma pack(push, 1)

struct I3Header
{
	uint32 signature;
	uint32 version;
	uint64 unk1[3];
	uint64 nSectors;
	uint64 ofsSectors;
	uint64 nPortals;
	uint64 ofsPortals;
	uint64 nPasses;
	uint64 ofsPasses;
	uint64 nUnk3;
	uint64 ofsUnk3; // 2 bytes per entry
	uint64 nUnk4;
	uint64 ofsUnk4; // 64 bytes per entry
	uint64 nUnk5;
	uint64 ofsUnk5; // 40 bytes per entry
	uint64 nUnk6;
	uint64 ofsUnk6; // 176 bytes per entry
	uint64 nUnk7;
	uint64 ofsUnk7; // 32 bytes per entry
	uint64 nUnk8;
	uint64 ofsUnk8; // 64 bytes per entry
	uint64 nUnk9;
	uint64 ofsUnk9; // 2 bytes per entry
	uint64 nUnk10;
	uint64 ofsUnk10; // 56 bytes per entry
	uint64 nUnk11;
	uint64 ofsUnk11; // 8 bytes per entry
	uint64 nUnk12;
	uint64 ofsUnk12; // 2 bytes per entry
	uint64 nUnk13;
	uint64 ofsUnk13; // 2 bytes per entry
	uint64 nUnk14;
	uint64 ofsUnk14; // 4 bytes per entry
	uint64 nUnk15;
	uint64 ofsUnk15; // 4 bytes per entry
	uint64 lenStringTable;
	uint64 ofsStringTable; // 2 bytes per entry
	uint64 nUnk17;
	uint64 ofsUnk17; // 4 bytes per entry
	float bboxMin[3];
	uint32 pad1;
	float bboxMax[3];
	uint32 pad2;
	uint64 unkVals[2];
}; // sizeof(I3Header) = 0x170

struct Unk5Entry
{
	uint64 unk1;
	uint64 lenName;
	uint64 ofsName;
	uint64 nUnk1; // 2 byte values -> probably string (all 0 so far)
	uint64 ofsUnk2;
};

struct Unk10Entry
{
	uint64 various1[3];
	uint64 nUnk101;
	uint64 ofsUnk101;
	uint64 nUnk102;
	uint64 ofsUnk102;
};

struct Unk101Entry
{
	float vertex1[3];
	float vertex2[3];
	float vertex3[3];
	float vertex4[4];
};

struct I3SectorEntry
{
	uint64 sectorNumber;
	uint64 unk1;
	float bboxMin[3];
	uint32 padding1;
	float bboxMax[3];
	uint32 padding2;
	uint64 unk2;
	uint64 unk3;
};

struct I3Portal
{
	uint32 unkNumber;
	uint32 flags; // maybe?
	uint64 unk1;
	float bboxMin[3]; uint32 padding1; // same for all portals, seems to be for whole model
	float bboxMax[3]; uint32 padding2; // same for all portals, seems to be for whole model
	float rotationQuaternion[4];
	uint64 nPortalVertices;
	uint64 ofsPortalVertices; // float[3] (mostly float[2] with one float == 0 as portals are axis aligned), uint32 always 0
	uint64 nPortalIndices;
	uint64 ofsPortalIndices; // points to 2 byte values, relative to sizeof(I3Header) + header.ofsUnk1 + header.nUnk1 * sizeof(Unk1Entry)
	float someMinValue[2]; uint32 padding3[2];
	float someMaxValue[2]; uint32 padding4[2];
	uint64 unk2;
	uint32 unk3;
	uint32 unk4;
};

struct I3Pass
{
	uint32 unk1; // always 0 so far
	uint32 unkCounter; // 0, 1, 40, 41, ...
	uint64 lenPassName; // -> 2 byte values
	union {
		uint64 ofsPassName;
		wchar_t* passName;
	};
	uint64 nUnk22; // -> 40 bytes values
	uint64 ofsUnk22;
	uint64 nUnk23; // -> 2 byte values
	uint64 ofsUnk23;
	uint64 nTexUnits; // -> 32 byte values
	uint64 ofsTexUnits;
	uint64 nUnk25;
	uint64 ofsUnk25; // -> 2 byte values
};

struct Unk22Entry
{
	uint32 unk1, unk2, unk3, unk4, unk5, unk6;
	uint64 nUnk221;
	uint64 ofsUnk221;
};

struct Unk221Entry
{
	uint16 texutreIndex, unk1;
	uint32 unk2, unk3, unk4, unk5, unk6;
	uint64 pairs[5][3];
	uint32 unk7, unk8, unk9, unk10, unk11, unk12;
	uint64 pairs2[5][3];
	uint32 unk13, unk14;
};

struct I3TexUnit
{
	uint16 unk1; // always 0xFF
	uint16 flags; // probably, 0 -> color texture, 1 -> normal texture
	uint32 unk3; // always 5 (also in the texture struct for M3 this is mostly 5)
	uint32 unk4;
	uint32 unk5;
	uint64 lenTextureName;
	union {
		uint64 ofsTextureName;
		wchar_t* textureName;
	};
};

#pragma pack(pop)

class I3ModelPass
{
	I3Pass mPass;
	std::vector<uint8>& mContent;
	std::shared_ptr<BinStream> mStream;
	uint64 mBaseOffset;
	std::vector<I3TexUnit> mTexUnits;
	std::vector<Unk22Entry> mUnk22Entries;
	std::vector<std::vector<Unk221Entry>> mUnk221Entries;

	void loadData();
public:
	I3ModelPass(I3Pass pass, std::shared_ptr<BinStream> strm, std::vector<uint8>& content, uint64 endOfBlock);

	uint32 getNumTextures() const { return mTexUnits.size(); }
	const I3TexUnit& getTexUnit(uint32 index) const { return mTexUnits[index]; }
	std::wstring getPassName() const { return mPass.passName; }
	std::vector<Unk221Entry>& getUnkEntries(uint32 index) { return mUnk221Entries[index]; }
};

SHARED_FWD(InputGeometry);
SHARED_FWD(TextureInput);

class I3Model : public std::enable_shared_from_this<I3Model>
{
	FileEntryPtr mFile;
	std::vector<uint8> mContent;
	std::shared_ptr<BinStream> mStream;
	std::vector<I3SectorEntry> mSectors;
	std::vector<I3Portal> mPortals;
	std::vector<std::unique_ptr<I3ModelPass>> mPasses;
	std::vector<uint16> mUnkIndices;
	std::vector<Unk5Entry> mUnk4Entries;
	std::vector<std::wstring> mStringTable;
	std::vector<I3SectorPtr> mSectorModels;
	std::vector<TexturePtr> mTextures;
	float mMaxDistance;
	float mHeight;

	GLuint mDisplayList;

	template<typename T>
	void read(std::vector<T>& dst, uint64 nElems, uint64 ofs) {
		mStream->seek(ofs + 0x170);
		dst.resize((uint32) nElems);
		mStream->read(dst.data(), sizeof(T) * nElems);
	}

	void loadSectors();

public:
	I3Model(FileEntryPtr file);

	uint32 getNumPasses() const { return mPasses.size(); }
	const std::unique_ptr<I3ModelPass>& getPass(uint32 passIndex) const { return mPasses[passIndex]; }

	float getMaxDistance() const { return mMaxDistance; }
	float getMaxHeight() const { return mHeight; }

	const std::vector<I3SectorPtr>& getSectors() const { return mSectorModels; }

	bool load();

	void render(InputGeometryPtr geom, TextureInputPtr texInput);
	void renderSector(uint32 sector, InputGeometryPtr geom, TextureInputPtr texInput);

	TexturePtr getTexture(uint32 index) { return mTextures[mPasses[0]->getUnkEntries(index)[0].texutreIndex]; }

	uint32 getNumTextures() { return mTextures.size(); }
};

SHARED_TYPE(I3Model);