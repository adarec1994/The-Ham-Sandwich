#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemRandomStat
    {
        uint32_t ID;
        uint32_t itemRandomStatGroupId;
        float weight;
        uint32_t itemStatTypeEnum;
        uint32_t itemStatData;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            itemRandomStatGroupId = file.getUint(recordIndex, i++);
            weight = file.getFloat(recordIndex, i++);
            itemStatTypeEnum = file.getUint(recordIndex, i++);
            itemStatData = file.getUint(recordIndex, i++);
        }
    };
}
