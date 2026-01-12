#pragma once

#include "Program.h"
#include "VertexElement.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

enum class VertexLayout
{
	Triangles = GL_TRIANGLES,
	TriangleStrip = GL_TRIANGLE_STRIP,
};

class InputGeometry
{
	std::list<VertexElementPtr> mElements;
	ProgramPtr mProgram;
	uint32 mVertexCount;
	uint32 mTriangleCount;
	uint32 mStride;
	uint32 mStartIndex;
	VertexLayout mLayout;
	VertexBufferPtr mVertexData;
	IndexBufferPtr mIndexData;
	uint32 mInstanceCount;

public:
	InputGeometry();

	uint32 getVertexCount() const { return mVertexCount; }
	uint32 getTriangleCount() const { return mTriangleCount; }
	ProgramPtr getProgram() const { return mProgram; }
	IndexBufferPtr getIndexBuffer() const { return mIndexData; }
	VertexBufferPtr getVertexBuffer() const { return mVertexData; }
	VertexLayout getLayout() const { return mLayout; }
	uint32 getStride() const { return mStride; }
	uint32 getStartIndex() const { return mStartIndex; }
	uint32 getInstanceCount() const { return mInstanceCount; }
	uint32 getIndexCount() const;

	void bindElements();
	void unbindElements();

	void addElement(VertexElementPtr element) { mElements.push_back(element); }
	void addElement(VertexSemantic semantic, uint32 index, uint32 numComponents) { addElement(std::make_shared<VertexElement>(semantic, index, numComponents)); }
	void setProgram(ProgramPtr program) { mProgram = program; }
	void setVertexCount(uint32 vertexCount) { mVertexCount = vertexCount; }
	void setTriangleCount(uint32 numTriangles) { mTriangleCount = numTriangles; }
	void setVertexLayout(VertexLayout layout) { mLayout = layout; }
	void setStride(uint32 stride) { mStride = stride; }
	void setStartIndex(uint32 index) { mStartIndex = index; }
	void setInstanceCount(uint32 instances) { mInstanceCount = instances; }

	void finalize();

	void setVertexBuffer(VertexBufferPtr vbuf) { mVertexData = vbuf; }
	void setIndexBuffer(IndexBufferPtr ibuf) { mIndexData = ibuf; }
};

typedef std::shared_ptr<InputGeometry> InputGeometryPtr;