#include "stdafx.h"
#include "InputGeometry.h"

InputGeometry::InputGeometry() {
	mVertexCount = 0;
	mTriangleCount = 0;
	mStride = 0;
	mVertexData = nullptr;
	mIndexData = nullptr;
	mLayout = VertexLayout::Triangles;
	mProgram = nullptr;
	mStartIndex = 0;
	mInstanceCount = 0;
}

void InputGeometry::finalize() {
	if (mProgram == nullptr)
		throw std::exception("mProgram == nullptr");

	uint32 offset = 0;

	for (auto& elem : mElements) {
		elem->bindToProgram(mProgram, mStride, offset);
		offset += elem->getByteSize();
	}
}

void InputGeometry::bindElements() {
	if (mProgram == nullptr)
		throw std::exception("mProgram == nullptr");

	if (mVertexData == nullptr)
		throw std::exception("mVertexData == nullptr");

	mVertexData->apply();

	for (auto& elem : mElements)
		elem->bindData(mProgram);
}

void InputGeometry::unbindElements() {
	if (mProgram == nullptr)
		throw std::exception("mProgram == nullptr");

	if (mVertexData == nullptr)
		throw std::exception("mVertexData == nullptr");

	mVertexData->remove();

	for (auto& elem : mElements)
		elem->unbindData(mProgram);
}

uint32 InputGeometry::getIndexCount() const {
	if (mLayout == VertexLayout::Triangles) {
		return mTriangleCount * 3;
	} else if (mLayout == VertexLayout::TriangleStrip) {
		return mTriangleCount + 2;
	} else {
		return 0;
	}
}