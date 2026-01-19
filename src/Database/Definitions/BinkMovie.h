#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct BinkMovie
    {
        uint32_t ID;
        std::wstring binkMovieAssetPath;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            binkMovieAssetPath = file.getString(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
