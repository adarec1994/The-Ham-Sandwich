#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathExplorerActivate
    {
        uint32_t ID;
        uint32_t creature2Id;
        uint32_t targetGroupId;
        uint32_t count;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2Id = file.getUint(recordIndex, i++);
            targetGroupId = file.getUint(recordIndex, i++);
            count = file.getUint(recordIndex, i++);
        }
    };
}
