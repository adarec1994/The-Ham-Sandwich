#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MaterialData
    {
        uint32_t ID;
        uint32_t row;
        uint32_t materialTypeId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            row = file.getUint(recordIndex, i++);
            materialTypeId = file.getUint(recordIndex, i++);
        }
    };
}
