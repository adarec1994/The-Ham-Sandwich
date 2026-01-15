#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PropAdditionalDetail
    {
        uint32_t ID;
        uint32_t localizedTextId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            localizedTextId = f.getUint(row, i++);
        }
    };
}
