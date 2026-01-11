#pragma once

#include "Program.h"

enum class VertexSemantic
{
	Position,
	TexCoord,
	Normal,
	BoneWeight,
	BoneIndex,
	Color
};

enum class DataType
{
	Float = GL_FLOAT,
	Byte = GL_UNSIGNED_BYTE
};

class VertexElement
{
	VertexSemantic mSemantic;
	DataType mType;
	uint32 mIndex;
	uint32 mComponents;
	uint32 mDataComponents;
	uint32 mAttribIndex;
	uint32 mDataOffset;
	uint32 mStride;
	ProgramPtr mBoundProgram;
	bool mNormalized;

public:
	VertexElement(VertexSemantic semantic, uint32 index, uint32 components, DataType type = DataType::Float, uint32 dataComponents = 0xFFFFFFFF);

	void bindToProgram(ProgramPtr prog, uint32 stride, uint32 offset);
	void bindData(ProgramPtr prog);
	void unbindData(ProgramPtr prog);
	void setNormalized(bool normalized) { mNormalized = normalized; }

	uint32 getByteSize() const;
};

typedef std::shared_ptr<VertexElement> VertexElementPtr;