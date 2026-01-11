#include "stdafx.h"
#include "Compression.h"

BinaryStreamPtr ZLib::inflate(BinaryStreamPtr data, uint32 numOut) {
	z_stream strm = { 0 };
	auto ret = inflateInit(&strm);
	if (ret != Z_OK) {
		v8::ThrowException(v8::String::New("RuntimeError: inflateInit failed."));
		return nullptr;
	}

	std::vector<uint8> chunk(numOut);
	uint32 processed = 0;

	strm.avail_in = data->size();
	strm.next_in = data->getPtr();

	strm.avail_out = numOut;
	strm.next_out = chunk.data();

	ret = ::inflate(&strm, Z_NO_FLUSH);
	if (ret == Z_STREAM_ERROR) {
		v8::ThrowException(v8::String::New("RuntimeError: inflate failed."));
		return nullptr;
	}

	inflateEnd(&strm);

	return std::make_shared<BinaryStream>(chunk);
}