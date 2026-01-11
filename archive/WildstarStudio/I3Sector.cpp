#include "stdafx.h"
#include "I3Sector.h"
#include "I3Model.h"

I3Sector::I3Sector(const std::wstring& sector, const std::wstring& fileName, std::weak_ptr<I3Model> parent) {
	mSectorName = sector;
	mParent = parent;

	mFile = std::dynamic_pointer_cast<FileEntry>(sIOMgr->getArchive()->getByPath(fileName));
	if (mFile == nullptr) {
		throw std::exception("File not found!");
	}
}

void I3Sector::load() {
	sIOMgr->getArchive()->getFileData(mFile, mContent);
	mStream = std::unique_ptr<BinStream>(new BinStream(mContent));

	I3SectorHeader header = mStream->read<I3SectorHeader>();

	read(mVertices, header.nVertices, header.ofsVertices);

	struct I3VertexInfo
	{
		int32 nx, ny, nz;
		uint16 halfu, halfv;
		uint16 halfs, halft;
		uint16 halfw, halfx;
		uint32 unk1, unk2;
	};

	std::vector<I3VertexInfo> vi;
	read(vi, header.nVertexInfo, header.ofsVertexInfo);
	read(mIndices, header.nIndices, header.ofsIndices);
	read(mSubMeshes, header.nSubMeshes, header.ofsSubMeshes);

	mNormals.resize(mVertices.size());
	std::vector<I3ModelVertex> modelVertices(mVertices.size());

	mMaxDistance = 0.0f;
	float minPos = FLT_MAX;
	float maxPos = -FLT_MAX;

	for (uint32 i = 0; i < mVertices.size(); ++i) {
		auto& mv = modelVertices[i];
		I3Vertex n;
		n.x = getFixedPoint(vi[i].nx);
		n.y = getFixedPoint(vi[i].ny);
		n.z = getFixedPoint(vi[i].nz);

		mv.x = mVertices[i].x;
		mv.y = mVertices[i].y;
		mv.z = mVertices[i].z;
		mv.nx = n.x;
		mv.ny = n.y;
		mv.nz = n.z;
		mv.u = DirectX::PackedVector::XMConvertHalfToFloat(vi[i].halfu);
		mv.v = DirectX::PackedVector::XMConvertHalfToFloat(vi[i].halfv);

		if (mv.y < minPos) {
			minPos = mv.y;
		}

		if (mv.y > maxPos) {
			maxPos = mv.z;
		}

		float len = sqrtf(mv.x * mv.x + mv.y * mv.y + mv.z * mv.z);
		if (len > mMaxDistance) {
			mMaxDistance = len;
		}

		mNormals[i] = n;
	}

	mVertexBuffer = std::make_shared<VertexBuffer>();
	mIndexBuffer = std::make_shared<IndexBuffer>();
	mIndexBuffer->setIndices(mIndices);
	mIndexBuffer->setIndexType(true);

	mVertexBuffer->setData(modelVertices);

	mHeight = abs(maxPos - minPos);
	mMinPos = maxPos < minPos ? maxPos : minPos;
}

void I3Sector::render(InputGeometryPtr geom, TextureInputPtr texInput) {
	geom->setVertexBuffer(mVertexBuffer);
	geom->setVertexCount(mVertices.size());
	geom->setIndexBuffer(mIndexBuffer);

	sPipeline->applyGeometry(geom);

	uint32 texMap[] = { 0, 2, 4, 6 };

	for (uint32 i = 0; i < 1; ++i) {
		auto& m = mSubMeshes[i];
		if (m.unk2 == (uint32) -1) {
			continue;
		}

		texInput->setTexture(L"_texture0", mParent.lock()->getTexture(m.unk2));
		geom->setStartIndex(m.startIndex);
		geom->setTriangleCount((m.endIndex - m.startIndex + 1) / 3);

		sPipeline->render(texInput);
	}

	sPipeline->removeGeometry(geom);
}