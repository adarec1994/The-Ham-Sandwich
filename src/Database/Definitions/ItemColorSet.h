#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemColorSet
    {
        uint32_t ID;
        uint32_t dyeColorRampId00;
        uint32_t dyeColorRampId01;
        uint32_t dyeColorRampId02;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            dyeColorRampId00 = file.getUint(recordIndex, i++);
            dyeColorRampId01 = file.getUint(recordIndex, i++);
            dyeColorRampId02 = file.getUint(recordIndex, i++);
        }
    };
}
