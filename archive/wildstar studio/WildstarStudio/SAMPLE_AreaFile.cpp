#include "stdafx.h"
#include "AreaFile.h"
#include "InputGeometry.h"
#include "Pipeline.h"
#include "UIManager.h"
#include "DataTable.h"
#include "TextureManager.h"
#include "TextureInput.h"

const float AreaFile::UnitSize = 2.0f;
InputGeometryPtr gAreaGeom = nullptr;
TextureInputPtr gAreaTex = nullptr;
uint32 AreaFile::gUniformProj = 0;
uint32 AreaFile::gUniformView = 0;
std::vector<uint32> AreaChunkRender::indices;
DataTablePtr gWorldLayer;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

AreaFile::AreaFile(const std::wstring& name, FileEntryPtr file) {
	if (gWorldLayer == nullptr) {
		gWorldLayer = std::make_shared<DataTable>(std::dynamic_pointer_cast<FileEntry>(sIOMgr->getArchive()->getByPath(L"DB\\WorldLayer.tbl")));
		gWorldLayer->initialLoadIDs();
	}

	mPath = file->getFullPath();
	mModelName = std::tr2::sys::path(name).filename();

	mFile = file;
	sIOMgr->getArchive()->getFileData(file, mContent);
	mStream = std::make_shared<BinStream>(mContent);
}

bool AreaFile::load() {
	if (gAreaGeom == nullptr) {
		initGeometry();
	}

	uint32 signature = mStream->read<uint32>();
	if (signature != 'area') {
		return false;
	}

	uint32 version = mStream->read<uint32>();
	if (version != 0) {
		return false;
	}

	std::map<uint32, std::vector<uint8>> chunks;
	while (mContent.size() - mStream->tell() >= 8) {
		uint32 magic = mStream->read<uint32>();
		uint32 size = mStream->read<uint32>();
		if (size + mStream->tell() > mContent.size()) {
			break;
		}

		std::vector<uint8> data(size);
		mStream->read(data.data(), size);
		chunks[magic] = data;
	}

	auto itr = chunks.find('CHNK');
	if (itr == chunks.end()) {
		return false;
	}
	
	auto strm = std::make_shared<BinStream>(itr->second);
	mChunks.resize(256);

	uint32 numValidChunks = 0;
	float totalHeight = 0.0f;

	uint32 lastIndex = 0;
	while (itr->second.size() - strm->tell() >= 4) {
		uint32 value = strm->read<uint32>();
		uint32 index = value >> 24;
		index += lastIndex;
		lastIndex = index + 1;
		uint32 size = (value & 0xFFFFFF);
		std::vector<uint8> data;
		if (index >= 256) {
			break;
		}

		if (strm->tell() + size > itr->second.size()) {
			break;
		}

		if (size < 4) {
			continue;
		}

		data.resize(size - 4);

		uint32 flags = strm->read<uint32>();

		strm->read(data.data(), data.size());

		mChunks[index] = std::make_shared<AreaChunkRender>(flags, data, (index % 16) * 16 * UnitSize, (index / 16) * 16 * UnitSize);
		auto& chunk = mChunks[index];
		if (chunk->hasHeightmap() && chunk->hasBlendValues() && chunk->hasTextureIds()) {
			if (mChunks[index]->getMaxHeight() > mMaxHeight) {
				mMaxHeight = mChunks[index]->getMaxHeight();
			}

			totalHeight += mChunks[index]->getAverageHeight();
			++numValidChunks;
		}
	}

	mAverageHeight = totalHeight / numValidChunks;

	return true;
}

bool AreaFile::loadForExport() {
	uint32 signature = mStream->read<uint32>();
	if (signature != 'area') {
		return false;
	}

	uint32 version = mStream->read<uint32>();
	if (version != 0) {
		return false;
	}

	std::map<uint32, std::vector<uint8>> chunks;
	while (mContent.size() - mStream->tell() >= 8) {
		uint32 magic = mStream->read<uint32>();
		uint32 size = mStream->read<uint32>();
		if (size + mStream->tell() > mContent.size()) {
			break;
		}

		std::vector<uint8> data(size);
		mStream->read(data.data(), size);
		chunks[magic] = data;
	}

	auto itr = chunks.find('CHNK');
	if (itr == chunks.end()) {
		return false;
	}

	auto strm = std::make_shared<BinStream>(itr->second);
	mChunks.resize(256);

	uint32 numValidChunks = 0;
	float totalHeight = 0.0f;

	uint32 lastIndex = 0;
	while (itr->second.size() - strm->tell() >= 4) {
		uint32 value = strm->read<uint32>();
		uint32 index = value >> 24;
		index += lastIndex;
		lastIndex = index + 1;
		uint32 size = (value & 0xFFFFFF);
		std::vector<uint8> data;
		if (index >= 256) {
			break;
		}

		if (strm->tell() + size > itr->second.size()) {
			break;
		}

		if (size < 4) {
			continue;
		}

		data.resize(size - 4);

		uint32 flags = strm->read<uint32>();

		strm->read(data.data(), data.size());

		mChunks[index] = std::make_shared<AreaChunkRender>(flags, data, (index % 16) * 16 * UnitSize, (index / 16) * 16 * UnitSize, true);
		auto& chunk = mChunks[index];
		if (chunk->hasHeightmap() && chunk->hasBlendValues() && chunk->hasTextureIds()) {
			if (mChunks[index]->getMaxHeight() > mMaxHeight) {
				mMaxHeight = mChunks[index]->getMaxHeight();
			}

			totalHeight += mChunks[index]->getAverageHeight();
			++numValidChunks;
		}
	}

	mAverageHeight = totalHeight / numValidChunks;

	return true;
}

void AreaFile::render(const Matrix& matView, const Matrix& matProj) {
	auto prog = gAreaGeom->getProgram();
	prog->begin();
	prog->set(gUniformProj, matProj);
	prog->set(gUniformView, matView);
	prog->end();

	for (auto& chunk : mChunks) {
		if (chunk != nullptr) {
			chunk->render();
		}
	}
}

void AreaFile::initGeometry() {
	gAreaGeom = std::make_shared<InputGeometry>();
	gAreaGeom->addElement(std::make_shared<VertexElement>(VertexSemantic::Position, 0, 3));
	gAreaGeom->addElement(std::make_shared<VertexElement>(VertexSemantic::Normal, 0, 3));
	gAreaGeom->addElement(std::make_shared<VertexElement>(VertexSemantic::Normal, 1, 4));
	gAreaGeom->addElement(std::make_shared<VertexElement>(VertexSemantic::TexCoord, 0, 2));
	gAreaGeom->setTriangleCount(16 * 16 * 4);
	gAreaGeom->setVertexCount(17 * 17 + 16 * 16);
	gAreaGeom->setVertexLayout(VertexLayout::Triangles);
	gAreaGeom->setStride(sizeof(AreaVertex));
	
	ProgramPtr prog = std::make_shared<Program>();
	prog->loadPixelShader("GLSL", "TERRAINFRAGMENT");
	prog->loadVertexShader("GLSL", "TERRAINVERTEX");
	prog->linkProgram();

	gAreaGeom->setProgram(prog);
	gAreaGeom->finalize();

	gUniformProj = prog->getUniformIndex(L"matProj");
	gUniformView = prog->getUniformIndex(L"matView");

	gAreaTex = std::make_shared<TextureInput>();
	gAreaTex->attachToProgram(prog);

	AreaChunkRender::geometryInit(prog);
}

void AreaFile::exportToObj(bool sync) {
	auto lambda = [this, sync]() {
		std::wstringstream strm;
		strm << sIOMgr->getExtractionPath() << mPath << L".obj";

		std::wstring dir = std::tr2::sys::path(strm.str()).parent_path().wstring();

		SHCreateDirectoryEx(nullptr, dir.c_str(), nullptr);

		std::ofstream os(strm.str());

		uint32 curIndex = 0;
		for (auto& chunk : mChunks) {
			if (chunk != nullptr) {
				chunk->exportToObj(os, curIndex);
			}
		}

		os.close();

		if (!sync) {
			sUIMgr->asyncExtractComplete();
		}
	};

	if (sync) {
		lambda();
	} else {
		std::async(std::launch::async, lambda);
	}
}

AreaChunkRender::AreaChunkRender(uint32 flags, const std::vector<uint8>& ac, float baseX, float baseY, bool objOnly) : mChunkData(ac), mFlags(flags) {
	if (!hasHeightmap() || !hasBlendValues() || !hasTextureIds()) {
		return;
	}

	if (!objOnly) {
		mVertexBuffer = std::make_shared<VertexBuffer>();
		mIndexBuffer = std::make_shared<IndexBuffer>();
	}

	if (indices.size() == 0) {
		indices.resize(16 * 16 * 4 * 3);

		for (uint32 i = 0; i < 16; ++i) {
			for (uint32 j = 0; j < 16; ++j) {
				uint32 tribase = (i * 16 + j) * 4 * 3;
				uint32 ibase = i * 33 + j;

				indices[tribase] = ibase;
				indices[tribase + 1] = ibase + 1;
				indices[tribase + 2] = ibase + 17;

				indices[tribase + 3] = ibase + 1;
				indices[tribase + 4] = ibase + 34;
				indices[tribase + 5] = ibase + 17;

				indices[tribase + 6] = ibase + 34;
				indices[tribase + 7] = ibase + 33;
				indices[tribase + 8] = ibase + 17;

				indices[tribase + 9] = ibase + 33;
				indices[tribase + 10] = ibase;
				indices[tribase + 11] = ibase + 17;
			}
		}
	}
#pragma pack(push, 1)
	struct HeightMap { uint16 data[19][19]; };
	struct BlendData { uint16 data[65][65]; };
	struct ColorData { uint16 data[65][65]; };
#pragma pack(pop)

	HeightMap& hm = *(HeightMap*) mChunkData.data();

	uint32 ofsTexIds = sizeof(HeightMap);
	uint32* textureIds = (uint32*) (mChunkData.data() + ofsTexIds);

	uint32 ofsBlend = sizeof(HeightMap) + 4 * sizeof(uint32);
	BlendData& blend = *(BlendData*) (mChunkData.data() + ofsBlend);

	ColorData* clrData = nullptr;

	bool colorData = false;
	if (hasColorMap()) {
		colorData = true;
		clrData = (ColorData*) (mChunkData.data() + ofsBlend + (65 * 65 * 2));
	}

	if (!objOnly) {
		mIndexBuffer->setIndexType(true);
		mIndexBuffer->setIndices(indices);
	}

	float totalHeight = 0.0f;
	float lastHeight = -FLT_MAX;
	uint32 numValid = 0;
	bool first = true;

	mVertices.resize(19 * 19);
	for (int32 y = -1; y < 18; ++y) {
		for (int32 x = -1; x < 18; ++x) {
			auto h = hm.data[y + 1][x + 1] & 0x7FFF;

			AreaVertex v;
			v.x = baseX + x * AreaFile::UnitSize;
			v.z = baseY + y * AreaFile::UnitSize;
			v.y = -2048.0f + h / 8.0f;
			v.u = x / 15.0f;
			v.v = y / 15.0f;

			if (v.y > mMaxHeight) {
				mMaxHeight = v.y;
			}

			if (first == false) {
				auto diff = std::abs(v.y - lastHeight);
				if (diff < 500.0f) {
					totalHeight += v.y;
					lastHeight = v.y;
					++numValid;
				}
			} else {
				first = false;
				totalHeight += v.y;
				++numValid;
			}

			mVertices[(y + 1) * 19 + x + 1] = v;
		}
	}

	totalHeight /= numValid;
	mAverageHeight = totalHeight;

	if (!objOnly) {
		// 32 or 40
		std::vector<WorldLayerEntry> layers(4);
		mBlendValues.resize(65 * 65);
		for (uint32 i = 0; i < 4; ++i) {
			if (textureIds[i] == 0) {
				mTextures.push_back(nullptr);
				continue;
			}
			if (!gWorldLayer->getRowById(textureIds[i], layers[i])) {
				mTextures.push_back(nullptr);
				continue;
			}

			((float*) &mTexScales)[i] = 1.0f / (layers[i].metersPerTexture / 32.0f);

			mTextures.push_back(sTexMgr->getTexture(layers[i].colorMapPath));
			mNormalTextures.push_back(sTexMgr->getTexture(layers[i].normalMapPath));
			mTextures.back()->setRepeat();
			mTextures.back()->setAndGenMipmap();
			mNormalTextures.back()->setRepeat();
			mNormalTextures.back()->setAndGenMipmap();

			uint32 curLayer = mTextures.size() - 1;
			for (uint32 j = 0; j < 65 * 65; ++j) {
				uint16 val = blend.data[j / 65][j % 65];
				uint32 value = (val & (0xF << (i * 4))) >> (i * 4);

				uint8 blend = (uint8) ((value / 15.0f) * 255.0f);
				mBlendValues[j] |= blend << (8 * i);
			}
		}
	}

	extendBuffer();
	calcNormals();
	calcTangentBitangent();

	if (!objOnly) {
		mVertexBuffer->setData(mVertices);

		mBlendTexture = Texture::fromMemory(65, 65, mBlendValues);
		if (hasColorMap()) {
			for (uint32 j = 0; j < 65 * 65; ++j) {
				uint16 value = clrData->data[j / 65][j % 65];
				uint32 r = value & 0x1F;
				uint32 g = (value >> 5) & 0x3F;
				uint32 b = (value >> 11) & 0x1F;
				uint32 a = 0xFF;
				r = (uint32) ((r / 31.0f) * 255.0f);
				g = (uint32) ((g / 63.0f) * 255.0f);
				b = (uint32) ((b / 31.0f) * 255.0f);
				mBlendValues[j] = 0xFF000000 | (b << 16) | (g << 8) | r;
			}
		}

		mBlendTexture2 = Texture::fromMemory(65, 65, mBlendValues);
	}
}

void AreaChunkRender::extendBuffer() {
	auto vertices = std::vector<AreaVertex>(17 * 17 + 16 * 16);
	mFullVertices = mVertices;

	for (uint32 i = 0; i < 17; ++i) {
		for (uint32 j = 0; j < 17; ++j) {
			auto idx = i * 33 + j;
			vertices[idx] = mVertices[(i + 1) * 19 + j + 1];
		}
	}

	for (uint32 i = 0; i < 16; ++i) {
		for (uint32 j = 0; j < 16; ++j) {
			auto idx = 17 + i * 33 + j;
			auto& tl = mVertices[(i + 1) * 19 + j + 1];
			auto& tr = mVertices[(i + 1) * 19 + j + 2];
			auto& bl = mVertices[(i + 2) * 19 + j + 1];
			auto& br = mVertices[(i + 2) * 19 + j + 2];

			AreaVertex v = { 0 };
			v.x = (tl.x + tr.x + bl.x + br.x) / 4.0f;
			v.y = (tl.y + tr.y + bl.y + br.y) / 4.0f;
			v.z = (tl.z + tr.z + bl.z + br.z) / 4.0f;
			v.u = (tl.u + tr.u) / 2.0f;
			v.v = (tl.v + bl.v) / 2.0f;

			vertices[idx] = v;
		}
	}

	mVertices = vertices;
}

void AreaChunkRender::calcTangentBitangent() {
	auto triCount = indices.size() / 3;
	std::vector<Vector3> tan1(mVertices.size());
	std::vector<Vector3> tan2(mVertices.size());

	for (uint32 i = 0; i < triCount; ++i) {
		auto i1 = indices[i * 3];
		auto i2 = indices[i * 3 + 1];
		auto i3 = indices[i * 3 + 2];

		auto& v1 = mVertices[i1];
		auto& v2 = mVertices[i2];
		auto& v3 = mVertices[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = v2.u - v1.u;
		float s2 = v3.u - v1.u;
		float t1 = v2.v - v1.v;
		float t2 = v3.v - v1.v;

		float r = 1.0f / (s1 * t2 - s2 * t1);
		Vector3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Vector3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (uint32 i = 0; i < mVertices.size(); ++i) {
		auto& v = mVertices[i];
		Vector3 n(v.nx, v.ny, v.nz);
		Vector3& t = tan1[i];
		Vector3& t2 = tan2[i];

		Vector3 tan = (t - n * n.dot(t)).normalized();
		float tanw = n.cross(t).dot(t2) < 0.0f ? 1.0f : -1.0f;

		v.tanx = tan.X;
		v.tany = tan.Y;
		v.tanz = tan.Z;
		v.tanw = tanw;
	}
}

void AreaChunkRender::calcNormals() {
	for (uint32 i = 1; i < 18; ++i) {
		for (uint32 j = 1; j < 18; ++j) {
			auto& tl = mFullVertices[(i - 1) * 19 + j - 1];
			auto& tr = mFullVertices[(i - 1) * 19 + j + 1];
			auto& br = mFullVertices[(i + 1) * 19 + j + 1];
			auto& bl = mFullVertices[(i + 1) * 19 + j - 1];
			auto& v = mFullVertices[i * 19 + j];

			Vector3 P1(tl.x, tl.y, tl.z);
			Vector3 P2(tr.x, tr.y, tr.z);
			Vector3 P3(br.x, br.y, br.z);
			Vector3 P4(bl.x, bl.y, bl.z);
			Vector3 vert(v.x, v.y, v.z);

			auto N1 = (P2 - vert).cross(P1 - vert);
			auto N2 = (P3 - vert).cross(P2 - vert);
			auto N3 = (P4 - vert).cross(P3 - vert);
			auto N4 = (P1 - vert).cross(P4 - vert);

			auto norm = N1 + N2 + N3 + N4;
			norm.normalize();
			norm *= -1;

			auto& vnew = mVertices[(i - 1) * 33 + j - 1];

			vnew.nx = norm.X;
			vnew.ny = norm.Y;
			vnew.nz = norm.Z;
		}
	}

	for (uint32 i = 0; i < 16; ++i) {
		for (uint32 j = 0; j < 16; ++j) {
			auto idx = 17 + i * 33 + j;
			auto& tl = mVertices[i * 33 + j];
			auto& tr = mVertices[i * 33 + j + 1];
			auto& bl = mVertices[(i + 1) * 33 + j];
			auto& br = mVertices[(i + 1) * 33 + j + 1];

			auto& v = mVertices[idx];
			v.nx = (tl.nx + tr.nx + bl.nx + br.nx) / 4.0f;
			v.ny = (tl.ny + tr.ny + bl.ny + br.ny) / 4.0f;
			v.nz = (tl.nz + tr.nz + bl.nz + br.nz) / 4.0f;
		}
	}
}

void AreaChunkRender::exportToObj(std::ofstream& os, uint32& index) {
	if (hasHeightmap() == false || hasBlendValues() == false || hasTextureIds() == false) {
		return;
	}

	for (auto& v : mVertices) {
		os << "v " << v.x << " " << v.y << " " << v.z << std::endl;
	}

	for (uint32 i = 0; i < 16 * 16 * 4; ++i) {
		auto base = i * 3;
		os << "f " << (indices[base] + index + 1) << " " << (indices[base + 1] + index + 1) << " " << (indices[base + 2] + index + 1) << std::endl;
	}

	index += mVertices.size();
}

void AreaChunkRender::geometryInit(ProgramPtr prog) {
	uniforms.alphaTexture = prog->getUniformIndex(L"alphaTexture");
	uniforms.textures[0] = prog->getUniformIndex(L"texture0");
	uniforms.textures[1] = prog->getUniformIndex(L"texture1");
	uniforms.textures[2] = prog->getUniformIndex(L"texture2");
	uniforms.textures[3] = prog->getUniformIndex(L"texture3");
	uniforms.colorTexture = prog->getUniformIndex(L"colorTexture");
	uniforms.hasColorMap = prog->getUniformIndex(L"hasColorMap");
	uniforms.texScale = prog->getUniformIndex(L"texScale");
}

void AreaChunkRender::render() {
	if (hasHeightmap() == false || hasBlendValues() == false || hasTextureIds() == false) {
		return;
	}

	gAreaGeom->setVertexBuffer(mVertexBuffer);
	gAreaGeom->setIndexBuffer(mIndexBuffer);

	for (uint32 i = 0; i < mTextures.size(); ++i) {
		gAreaTex->setTexture(uniforms.textures[i], mTextures[i]);
	}

	for (uint32 i = mTextures.size(); i < 4; ++i) {
		gAreaTex->setTexture(uniforms.textures[i], nullptr);
	}

	if (hasColorMap()) {
		gAreaTex->setTexture(uniforms.colorTexture, mBlendTexture2);
	}

	auto& prog = gAreaGeom->getProgram();
	prog->begin();
	prog->set(uniforms.hasColorMap, hasColorMap() ? 1.0f : 0.0f);
	prog->set(uniforms.texScale, mTexScales);
	prog->end();

	gAreaTex->setTexture(uniforms.alphaTexture, mBlendTexture);

	sPipeline->applyGeometry(gAreaGeom);
	sPipeline->render(gAreaTex);
	sPipeline->removeGeometry(gAreaGeom);
}

uint32 AreaChunkRender::getTextureId(uint32 index) const {
	if (!hasTextureIds()) {
		throw std::bad_function_call();
	}

	uint32 offset = 0;
	if (hasHeightmap()) {
		offset += 19 * 19 * 2;
	}

	std::vector<uint32> data(4);
	memcpy(data.data(), mChunkData.data() + offset, data.size() * 4);
	return data[index];
}

std::vector<uint8> AreaChunkRender::getColorMap() const {
	if (hasColorMap() == false) {
		throw std::bad_function_call();
	}

	uint32 offset = 0;
	if (hasHeightmap()) {
		offset += 19 * 19 * 2;
	}

	if (hasTextureIds()) {
		offset += 4 * 4;
	}

	if (hasBlendValues()) {
		offset += 65 * 65 * 2;
	}

	std::vector<uint8> data(65 * 65 * 2);
	memcpy(data.data(), mChunkData.data() + offset, data.size());
	return data;
}

std::vector<uint8> AreaChunkRender::getBlendData() const {
	if (!hasBlendValues()) {
		throw std::bad_function_call();
	}

	uint32 offset = 0;
	if (hasHeightmap()) {
		offset += 19 * 19 * 2;
	}

	if (hasTextureIds()) {
		offset += 4 * 4;
	}

	std::vector<uint8> data(65 * 65 * 2);
	memcpy(data.data(), mChunkData.data() + offset, data.size());
	return data;
}

uint32 AreaChunkRender::getUnk7(uint32 index) const {
	if (hasUnk7() == false) {
		throw std::bad_function_call();
	}

	uint32 offset = 0;
	if (hasHeightmap()) {
		offset += 19 * 19 * 2;
	}

	if (hasTextureIds()) {
		offset += 4 * 4;
	}

	if (hasBlendValues()) {
		offset += 65 * 65 * 2;
	}

	if (hasColorMap()) {
		offset += 65 * 65 * 2;
	}

	if (hasUnk1()) {
		offset += 80;
	}

	if (hasShadowMap()) {
		offset += 65 * 65;
	}

	if (hasUnk2()) {
		offset += 64 * 64;
	}

	if (hasUnk3()) {
		offset += 1;
	}

	if (hasUnk4()) {
		offset += 4 * 4;
	}

	if (hasUnk5()) {
		offset += 0x5344;
	}

	if (hasUnk6()) {
		offset += 64 * 64;
	}

	std::vector<uint32> data(4);
	memcpy(data.data(), mChunkData.data() + offset, data.size() * 4);
	return data[index];
}