#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MaterialRemap
    {
        uint32_t ID;
        uint32_t materialDataRow;
        uint32_t materialSetId;
        uint32_t materialDataRowRemap;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            materialDataRow = file.getUint(recordIndex, i++);
            materialSetId = file.getUint(recordIndex, i++);
            materialDataRowRemap = file.getUint(recordIndex, i++);
        }
    };
}
