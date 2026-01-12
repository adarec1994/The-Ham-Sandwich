#pragma once

#include "GlExt.h"

class Buffer
{
protected:
	GLuint mBuffer;
	bool mIndex;

	virtual GLenum getTarget() const;

	virtual void onBufferDataChanged() { }

	Buffer(const Buffer&) { }
	void operator = (const Buffer&) { }
public:
	Buffer(bool index = false);
	virtual ~Buffer();

	void setBufferData(const void* data, uint32 numBytes);
	void apply();
	void remove();

	template<typename T>
	void setData(const std::vector<T>& data) {
		setBufferData(data.data(), data.size() * sizeof(T));
	}

	template<typename T, uint32 size>
	void setData(T(&data)[size]) {
		setBufferData(data, size * sizeof(T));
	}

	template<typename _FwdItr>
	void setData(_FwdItr begin, _FwdItr end) {
		setBufferData(&(*begin), (end - begin) * sizeof(_FwdItr::value_type));
	}
};