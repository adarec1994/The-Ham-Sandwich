#pragma once

#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
	VertexBuffer() : Buffer(false) {
	}
};

typedef std::shared_ptr<VertexBuffer> VertexBufferPtr;