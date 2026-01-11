#include "stdafx.h"
#include "VertexElement.h"
#include "GlExt.h"

VertexElement::VertexElement(VertexSemantic semantic, uint32 index, uint32 components, DataType type, uint32 dataComponents) :
mSemantic(semantic),
mIndex(index),
mComponents(components),
mDataComponents(dataComponents),
mAttribIndex(0),
mDataOffset(0),
mType(type),
mNormalized(false) {

}

void VertexElement::bindToProgram(ProgramPtr prog, uint32 stride, uint32 offset) {
	mBoundProgram = prog;
	mDataOffset = offset;
	mStride = stride;

	std::stringstream attribName;

	switch (mSemantic) {
	case VertexSemantic::Position:
		attribName << "position";
		break;

	case VertexSemantic::Normal:
		attribName << "normal";
		break;

	case VertexSemantic::TexCoord:
		attribName << "texcoord";
		break;

	case VertexSemantic::BoneIndex:
		attribName << "boneIndex";
		break;

	case VertexSemantic::BoneWeight:
		attribName << "boneWeight";
		break;

	case VertexSemantic::Color:
		attribName << "color";
		break;

	default:
		throw std::exception("Invalid semantic type");
	}

	attribName << mIndex;

	mAttribIndex = glGetAttribLocation(prog->getId(), attribName.str().c_str());
}

void VertexElement::bindData(ProgramPtr prog) {
	glEnableVertexAttribArray(mAttribIndex);
	glVertexAttribPointer(mAttribIndex, mComponents, (GLenum) mType, mNormalized, mStride, (const GLvoid*) mDataOffset);
}

void VertexElement::unbindData(ProgramPtr prog) {
	glDisableVertexAttribArray(mAttribIndex);
}

uint32 VertexElement::getByteSize() const {
	return mComponents * (mType == DataType::Byte ? 1 : sizeof(float));
}