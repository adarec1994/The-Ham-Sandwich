#include "StdAfx.h"
#include "Buffer.h"
#include "GlExt.h"

Buffer::Buffer(bool index) : mIndex(index) {
	glGenBuffers(1, &mBuffer);
}

Buffer::~Buffer() {
	glDeleteBuffers(1, &mBuffer);
}

GLenum Buffer::getTarget() const {
	return (mIndex ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER);
}

void Buffer::apply() {
	glBindBuffer(getTarget(), mBuffer);
}

void Buffer::remove() {
	glBindBuffer(getTarget(), 0);
}

void Buffer::setBufferData(const void* data, uint32 size) {
	apply();

	glBufferData(getTarget(), (GLsizeiptr) size, data, GL_STATIC_DRAW);

	remove();

	onBufferDataChanged();
}