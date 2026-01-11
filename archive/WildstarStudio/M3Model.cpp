#include "stdafx.h"
#include "M3Model.h"
#include "UIManager.h"
#include "TextureManager.h"

#define END_OF_HEADER 0x650

M3Model::M3Model(const std::wstring& fileName, FileEntryPtr file) {
	sIOMgr->getArchive()->getFileData(file, mContent);
	mPath = file->getFullPath();

	mStream = std::unique_ptr<BinStream>(new BinStream(mContent));

	mModelName = std::tr2::sys::path(fileName).filename();
}

void M3Model::loadForExport() {
	mHeader = mStream->read<M3Header>();

	mStream->seek(END_OF_HEADER + mHeader.ofsVertices);
	mVertices.resize((uint32) mHeader.nVertices);

	mStream->read(mVertices.data(), mVertices.size() * sizeof(M3Vertex));

	std::vector<uint32> indices((uint32) mHeader.nIndices);
	mStream->seek(END_OF_HEADER + mHeader.ofsIndices);
	mStream->read(indices.data(), indices.size() * sizeof(uint32));

	std::vector<M3SubMesh> meshes((uint32) mHeader.nSubMeshes);
	mStream->seek(END_OF_HEADER + mHeader.ofsSubMeshes);
	mStream->read(meshes.data(), meshes.size() * sizeof(M3SubMesh));

	mMeshes = meshes;

	std::vector<M3Skin> skins((uint32) mHeader.nViews);
	mStream->seek(END_OF_HEADER + mHeader.ofsViews);
	mStream->read(skins.data(), skins.size() * sizeof(M3Skin));

	std::vector<M3Texture> textures((uint32) mHeader.nTextures);
	mStream->seek(END_OF_HEADER + mHeader.ofsTextures);
	mStream->read(textures.data(), textures.size() * sizeof(M3Texture));

	uint64 texEnd = END_OF_HEADER + mHeader.ofsTextures + mHeader.nTextures * sizeof(M3Texture);
	texEnd = (texEnd + 15) & ~(uint64) 15;
	for (uint32 i = 0; i < textures.size(); ++i) {
		textures[i].name = (wchar_t*) &mContent[(uint32) (texEnd + textures[i].ofsName)];
	}

	mMaterials.resize((uint32) mHeader.nMaterials);
	mStream->seek(END_OF_HEADER + mHeader.ofsMaterials);
	mStream->read(mMaterials.data(), mMaterials.size() * sizeof(M3Material));

	uint64 materialEnd = END_OF_HEADER + mHeader.ofsMaterials + mMaterials.size() * sizeof(M3Material);
	materialEnd = (materialEnd + 15) & ~(uint64) 15;

	mTextures.resize(mMaterials.size());

	for (uint32 i = 0; i < mMaterials.size(); ++i) {
		mStream->seek(materialEnd + mMaterials[i].ofsTextures);
		int16 texId = mStream->read<int16>();
		if (texId < 0) {
			texId = mStream->read<int16>();
			if (texId < 0) {
				continue;
			}
		}

		mTextures[i] = std::make_shared<Texture>(std::dynamic_pointer_cast<FileEntry>(sIOMgr->getArchive()->getByPath(textures[texId].name)));
	}

	uint32 ofsVertex = (uint32) (END_OF_HEADER + mHeader.ofsViews + skins[0].sizeOfStruct + skins[0].ofsVertexLookup);
	uint32 ofsIndices = (uint32) (END_OF_HEADER + mHeader.ofsViews + skins[0].sizeOfStruct + skins[0].ofsIndexLookup);

	uint32* vertexLookup = (uint32*) &mContent[ofsVertex];
	uint32* indexLookup = (uint32*) &mContent[ofsIndices];

	mMaxDistance = 0.0f;
	float minPos = FLT_MAX;
	float maxPos = -FLT_MAX;

	mModelVertices.resize((uint32) mHeader.nVertices);

	std::vector<M3Vertex> vertices(mVertices.size());
	for (uint32 i = 0; i < vertices.size(); ++i) {
		auto& v = mVertices[i];
		float dist = sqrtf(v.x * v.x + v.z + v.z);
		if (dist > mMaxDistance)
			mMaxDistance = dist;

		if (-v.y > maxPos)
			maxPos = -v.y;
		if (-v.y < minPos)
			minPos = -v.y;

		vertices[i] = mVertices[vertexLookup[i]];
		auto& vertex = mVertices[vertexLookup[i]];
		auto& mv = mModelVertices[i];
		mv.x = vertex.x;
		mv.y = -vertex.y;
		mv.z = vertex.z;
		mv.nx = vertex.normals[0] / 127.0f;
		mv.ny = vertex.normals[1] / -127.0f;
		mv.nz = vertex.normals[2] / 127.0f;
		mv.u = DirectX::PackedVector::XMConvertHalfToFloat(vertex.s);
		mv.v = DirectX::PackedVector::XMConvertHalfToFloat(vertex.t);
	}

	mHeight = maxPos - minPos;

	mVertices = vertices;

	Triangle curTri;
	std::vector<Triangle> curMesh;

	for (uint32 i = 0; i < meshes.size(); ++i) {
		curMesh.clear();

		std::vector<uint32> curIndices;

		for (uint32 j = 0; j < meshes[i].nIndices; ++j) {
			uint32 index = indexLookup[j + meshes[i].startIndex] + meshes[i].startVertex;
			curIndices.push_back(index);
			curTri.indices[j % 3] = index;

			if ((j % 3) == 2) {
				curMesh.push_back(curTri);
			}
		}

		mMeshTriangles.push_back(curMesh);
	}
}

void M3Model::load() {
	mHeader = mStream->read<M3Header>();

	mStream->seek(END_OF_HEADER + mHeader.ofsVertices);
	mVertices.resize((uint32) mHeader.nVertices);

	mStream->read(mVertices.data(), mVertices.size() * sizeof(M3Vertex));

	std::vector<uint32> indices((uint32)mHeader.nIndices);
	mStream->seek(END_OF_HEADER + mHeader.ofsIndices);
	mStream->read(indices.data(), indices.size() * sizeof(uint32));

	std::vector<M3SubMesh> meshes((uint32)mHeader.nSubMeshes);
	mStream->seek(END_OF_HEADER + mHeader.ofsSubMeshes);
	mStream->read(meshes.data(), meshes.size() * sizeof(M3SubMesh));

	mMeshes = meshes;

	std::vector<M3Skin> skins((uint32) mHeader.nViews);
	mStream->seek(END_OF_HEADER + mHeader.ofsViews);
	mStream->read(skins.data(), skins.size() * sizeof(M3Skin));

	std::vector<M3Texture> textures((uint32) mHeader.nTextures);
	mStream->seek(END_OF_HEADER + mHeader.ofsTextures);
	mStream->read(textures.data(), textures.size() * sizeof(M3Texture));

	uint64 texEnd = END_OF_HEADER + mHeader.ofsTextures + mHeader.nTextures * sizeof(M3Texture);
	texEnd = (texEnd + 15) & ~(uint64) 15;
	for (uint32 i = 0; i < textures.size(); ++i) {
		textures[i].name = (wchar_t*) &mContent[(uint32) (texEnd + textures[i].ofsName)];
	}

	mMaterials.resize((uint32) mHeader.nMaterials);
	mStream->seek(END_OF_HEADER + mHeader.ofsMaterials);
	mStream->read(mMaterials.data(), mMaterials.size() * sizeof(M3Material));

	uint64 materialEnd = END_OF_HEADER + mHeader.ofsMaterials + mMaterials.size() * sizeof(M3Material);
	materialEnd = (materialEnd + 15) & ~(uint64) 15;

	mTextures.resize(mMaterials.size());
	
	for (uint32 i = 0; i < mMaterials.size(); ++i) {
		mStream->seek(materialEnd + mMaterials[i].ofsTextures);
		int16 texId = mStream->read<int16>();
		if (texId < 0) {
			texId = mStream->read<int16>();
			if (texId < 0) {
				continue;
			}
		}

		mTextures[i] = sTexMgr->getTexture(textures[texId].name);
	}

	uint32 ofsVertex = (uint32) (END_OF_HEADER + mHeader.ofsViews + skins[0].sizeOfStruct + skins[0].ofsVertexLookup);
	uint32 ofsIndices = (uint32) (END_OF_HEADER + mHeader.ofsViews + skins[0].sizeOfStruct + skins[0].ofsIndexLookup);

	uint32* vertexLookup = (uint32*) &mContent[ofsVertex];
	uint32* indexLookup = (uint32*) &mContent[ofsIndices];

	mMaxDistance = 0.0f;
	float minPos = FLT_MAX;
	float maxPos = -FLT_MAX;

	mModelVertices.resize((uint32) mHeader.nVertices);

	std::vector<M3Vertex> vertices(mVertices.size());
	for (uint32 i = 0; i < vertices.size(); ++i) {
		auto& v = mVertices[i];
		float dist = sqrtf(v.x * v.x + v.z + v.z);
		if (dist > mMaxDistance)
			mMaxDistance = dist;

		if (-v.y > maxPos)
			maxPos = -v.y;
		if (-v.y < minPos)
			minPos = -v.y;

		vertices[i] = mVertices[vertexLookup[i]];
		auto& vertex = mVertices[vertexLookup[i]];
		auto& mv = mModelVertices[i];
		mv.x = vertex.x;
		mv.y = -vertex.y;
		mv.z = vertex.z;
		mv.nx = vertex.normals[0] / 127.0f;
		mv.ny = vertex.normals[1] / -127.0f;
		mv.nz = vertex.normals[2] / 127.0f;
		mv.u = DirectX::PackedVector::XMConvertHalfToFloat(vertex.s);
		mv.v = DirectX::PackedVector::XMConvertHalfToFloat(vertex.t);
	}

	mVertexBuffer = std::make_shared<VertexBuffer>();
	mVertexBuffer->setData(mModelVertices);

	mHeight = maxPos - minPos;

	mVertices = vertices;

	Triangle curTri;
	std::vector<Triangle> curMesh;

	for (uint32 i = 0; i < meshes.size(); ++i) {
		curMesh.clear();

		std::vector<uint32> curIndices;

		for (uint32 j = 0; j < meshes[i].nIndices; ++j) {
			uint32 index = indexLookup[j + meshes[i].startIndex] + meshes[i].startVertex;
			curIndices.push_back(index);

			auto& v = vertices[index];

			auto& vt = curTri.vertices[j % 3];
			vt.x = v.x;
			vt.y = -v.y;
			vt.z = v.z;
			curTri.indices[j % 3] = index;
			auto& vn = curTri.normals[j % 3];
			vn.x = v.unk[0] / 127.0f;
			vn.y = v.unk[1] / 127.0f;
			vn.z = v.unk[2] / 127.0f;
			vn = vn.normalized();

			if ((j % 3) == 2) {
				curMesh.push_back(curTri);
			}
		}

		mMeshTriangles.push_back(curMesh);

		IndexBufferPtr ibuff = std::make_shared<IndexBuffer>();
		ibuff->setIndices(curIndices);
		ibuff->setIndexType(true);
		mMeshIndices.push_back(ibuff);
	}
}

void M3Model::computeNormals() {
	for (auto& mesh : mMeshTriangles) {
		for (auto& tri : mesh) {
			Vector& a = tri.vertices[0];
			Vector& b = tri.vertices[1];
			Vector& c = tri.vertices[2];

			Vector add = Vector::add(Vector::add(a, b), c);
			add.x /= 3;
			add.y /= 3;
			add.z /= 3;
			tri.center = add;

			Vector n = Vector::cross(Vector::sub(b, a), Vector::sub(c, a));
			n = n.normalized();
			tri.normal = n;
		}
	}
}

void M3Model::render(InputGeometryPtr geom, TextureInputPtr tex) {
	//glCallList(mDisplayList + (usePoints ? 1 : 0));
	
	//if (useNormal) {
	//	glCallList(mDisplayList + 2);
	//}

	geom->setVertexBuffer(mVertexBuffer);
	geom->setVertexCount(mModelVertices.size());

	sPipeline->applyGeometry(geom);

	for (uint32 i = 0; i < mMeshes.size(); ++i) {
		geom->setIndexBuffer(mMeshIndices[i]);
		geom->setTriangleCount(mMeshes[i].nIndices / 3);
		tex->setTexture(L"_texture0", mTextures[mMeshes[i].material]);

		sPipeline->render(tex);
	}

	sPipeline->removeGeometry(geom);
}

void M3Model::exportAsObj(bool sync) {
	auto lambda = [this, sync]() {
		std::wstringstream strm;
		strm << sIOMgr->getExtractionPath() << mPath << L".obj";

		std::wstring dir = std::tr2::sys::path(strm.str()).parent_path();

		SHCreateDirectoryEx(nullptr, dir.c_str(), nullptr);

		std::wstringstream mtlStrm;
		mtlStrm << sIOMgr->getExtractionPath() << mPath << L".mtl";

		std::ofstream os(mtlStrm.str());
		for (uint32 i = 0; i < mTextures.size(); ++i) {
			if (mTextures[i] == nullptr || mTextures[i]->getFile() == nullptr) {
				continue;
			}

			os << "newmtl texMat" << i << std::endl;
			os << "Ka 1.000 1.000 1.000" << std::endl;
			os << "Kd 1.000 1.000 1.000" << std::endl;
			os << "Ks 0.000 0.000 0.000" << std::endl;
			os << "illum 0" << std::endl;
			auto texName = mTextures[i]->getFile()->getEntryName();
			std::wstringstream texStrm;
			texStrm << dir << L"\\" << texName << L".bmp";
			CLSID clsid;
			GetEncoderClsid(L"image/bmp", &clsid);
			auto bmp = mTextures[i]->getBitmap();
			bmp->Save(texStrm.str().c_str(), &clsid);
			os << "map_Ka " << toMultibyte(texName) << ".bmp" << std::endl;
			os << "map_Kd " << toMultibyte(texName) << ".bmp" << std::endl;
			os << std::endl;
		}

		os.close();

		os = std::ofstream(strm.str());
		os << "mtllib " << toMultibyte(std::tr2::sys::path(mtlStrm.str()).filename()) << std::endl;

		for (auto& v : mVertices) {
			os << "v " << v.x << " " << v.y << " " << v.z << std::endl;
		}

		for (auto& v : mVertices) {
			os << "vt " << DirectX::PackedVector::XMConvertHalfToFloat(v.s) << " " << (1.0f - DirectX::PackedVector::XMConvertHalfToFloat(v.t)) << std::endl;
		}

		uint32 meshIndex = 0;
		for (auto& mesh : mMeshTriangles) {
			os << "usemtl texMat" << mMeshes[meshIndex++].material << std::endl;
			for (auto& tri : mesh) {
				os << "f " <<
					(tri.indices[0] + 1) << "/" << (tri.indices[0] + 1) << " " <<
					(tri.indices[1] + 1) << "/" << (tri.indices[1] + 1) << " " <<
					(tri.indices[2] + 1) << "/" << (tri.indices[2] + 1) << std::endl;
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