#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundEnvironment
    {
        uint32_t ID;
        uint32_t hash;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            hash = f.getUint(row, i++);
        }
    };
}
