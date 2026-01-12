#pragma once

#include "BinaryStream.h"

class ZLib
{
public:
	static BinaryStreamPtr inflate(BinaryStreamPtr data, uint32 numBytes);
};

SHARED_TYPE(ZLib);