#pragma once
#include <cstdint>
#include <vector>
#include "tex.h"

namespace Tex
{
    namespace Jpeg
    {
        bool Decode(const Header& header, int mipLevel, const std::vector<uint8_t>& data, std::vector<uint8_t>& outRGBA, int& outW, int& outH);
    }
}