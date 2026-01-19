#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemProficiency
    {
        uint32_t ID;
        uint32_t bitMask;
        uint32_t localizedTextIdString;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            bitMask = file.getUint(recordIndex, i++);
            localizedTextIdString = file.getUint(recordIndex, i++);
        }
    };
}
