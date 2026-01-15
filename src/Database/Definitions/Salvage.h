#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Salvage
    {
        uint32_t ID;
        uint32_t item2TypeId;
        uint32_t level;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            item2TypeId = f.getUint(row, i++);
            level = f.getUint(row, i++);
        }
    };
}
