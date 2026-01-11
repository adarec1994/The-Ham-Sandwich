#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
	GLenum mIndexType;
public:
	IndexBuffer() : Buffer(true) {
		mIndexType = GL_UNSIGNED_INT;
	}

	template<uint32 size>
	void setIndices(uint32(&indices)[size]) {
		setData(indices);
	}

	template<uint32 size>
	void setIndices(uint16(&indices)[size]) {
		setData(indices);
	}

	void setIndices(const std::vector<uint32>& data) {
		setData(data);
	}

	void setIndices(const std::vector<uint16>& data) {
		setData(data);
	}

	void setIndexType(bool _32Bit) {
		mIndexType = (GLenum) (_32Bit ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT);
	}

	uint32 getIndexSize() {
		return (mIndexType == GL_UNSIGNED_INT) ? 4 : 2;
	}

	GLenum getIndexType() { return mIndexType; }
};

typedef std::shared_ptr<IndexBuffer> IndexBufferPtr;