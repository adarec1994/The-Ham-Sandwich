#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SalvageException
    {
        uint32_t ID;
        uint32_t item2Id;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            item2Id = f.getUint(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
