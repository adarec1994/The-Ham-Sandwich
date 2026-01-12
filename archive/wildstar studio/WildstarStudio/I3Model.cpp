#include "stdafx.h"
#include "I3Model.h"
#include "TextureManager.h"

I3Model::I3Model(FileEntryPtr file) {
	mFile = file;
	sIOMgr->getArchive()->getFileData(file, mContent);
	mStream = std::make_shared<BinStream>(mContent);
}

bool I3Model::load() {
	I3Header header = mStream->read<I3Header>();
	if (header.signature != 0x49444546) {
		return false;
	}

	read(mSectors, header.nSectors, header.ofsSectors);
	read(mPortals, header.nPortals, header.ofsPortals);

	std::vector<I3Pass> passes;
	read(passes, header.nPasses, header.ofsPasses);

	read(mUnkIndices, header.nUnk3, header.ofsUnk3);
	std::vector<wchar_t> strTable;
	read(strTable, header.lenStringTable, header.ofsStringTable);
	std::vector<wchar_t>::iterator itr = strTable.begin();
	std::vector<wchar_t>::iterator found = strTable.begin();
	while ((found = std::find(itr, strTable.end(), L'\0')) != strTable.end()) {
		mStringTable.push_back(&(*itr));
		itr = found + 1;
	}

	for (uint32 i = 0; i < passes.size(); ++i) {
		uint64 baseOffset = 0x170 + header.ofsPasses + header.nPasses * sizeof(I3Pass);
		baseOffset = (baseOffset + 15) & ~(uint64) 15;
		uint64 offset = baseOffset + passes[i].ofsPassName;
		wchar_t* ptr = (wchar_t*) &mContent[(uint32) offset];
		passes[i].passName = ptr;

		mPasses.push_back(std::unique_ptr<I3ModelPass>(new I3ModelPass(passes[i], mStream, mContent, baseOffset)));
	}

	for (uint32 i = 0; i < mPasses[0]->getNumTextures(); ++i) {
		mTextures.push_back(sTexMgr->getTexture(mPasses[0]->getTexUnit(i).textureName));
	}

	mDisplayList = glGenLists(1);
	
	loadSectors();

	return true;
}

void I3Model::render(InputGeometryPtr geom, TextureInputPtr texInput) {
	for (auto& sec : mSectorModels) {
		sec->render(geom, texInput);
	}
	//glCallList(mDisplayList);
}

void I3Model::renderSector(uint32 sector, InputGeometryPtr geom, TextureInputPtr texInput) {
	if (sector >= mSectorModels.size()) {
		return;
	}

	mSectorModels[sector]->render(geom, texInput);
}

void I3Model::loadSectors() {
	std::wstring baseName = std::tr2::sys::path(mFile->getFullPath()).replace_extension(L".").wstring();
	mMaxDistance = 0.0f;
	mHeight = 0.0f;

	for (uint32 i = 0; i < mSectors.size(); ++i) {
		std::wstring sectorName = mStringTable[(uint32)mSectors[i].sectorNumber];
		std::wstringstream strm;
		strm << baseName << sectorName << L".i3";

		std::wstring sectorPath = strm.str();
		std::replace(sectorPath.begin(), sectorPath.end(), L'/', L'\\');
		auto sec = std::make_shared<I3Sector>(sectorName, sectorPath, shared_from_this());
		sec->load();

		if (sec->getHeight() > mHeight) {
			mHeight = sec->getHeight();
		}

		if (sec->getMaxDistance() > mMaxDistance) {
			mMaxDistance = sec->getMaxDistance();
		}

		mSectorModels.push_back(sec);
	}
}

I3ModelPass::I3ModelPass(I3Pass pass, std::shared_ptr<BinStream> strm, std::vector<uint8>& content, uint64 endOfBlock) : mContent(content) {
	mStream = strm;
	mPass = pass;
	mBaseOffset = endOfBlock;

	mTexUnits.resize((uint32) pass.nTexUnits);
	mUnk22Entries.resize((uint32) pass.nUnk22);
	mStream->seek(mBaseOffset + mPass.ofsUnk22);
	mStream->read(mUnk22Entries.data(), mUnk22Entries.size() * sizeof(Unk22Entry));
	for (uint32 i = 0; i < mUnk22Entries.size(); ++i) {
		uint64 offset = (mBaseOffset + mPass.ofsUnk22 + mPass.nUnk22 * sizeof(Unk22Entry) + 15) & ~(uint64) 15;
		offset += mUnk22Entries[i].ofsUnk221;
		std::vector<Unk221Entry> entries((uint32)mUnk22Entries[i].nUnk221);
		mStream->seek(offset);
		mStream->read(entries.data(), entries.size() * sizeof(Unk221Entry));
		mUnk221Entries.push_back(entries);
	}
	loadData();
}

void I3ModelPass::loadData() {
	mStream->seek((mBaseOffset + mPass.ofsTexUnits + 15) & ~(uint64) 15);
	mStream->read(mTexUnits.data(), mTexUnits.size() * sizeof(I3TexUnit));

	for (uint32 i = 0; i < mTexUnits.size(); ++i) {
		auto& unit = mTexUnits[i];
		wchar_t* ptr = (wchar_t*) &mContent[(uint32) (mBaseOffset + mPass.ofsTexUnits + mPass.nTexUnits * sizeof(I3TexUnit) + unit.ofsTextureName)];
		unit.textureName = ptr;
	}
}