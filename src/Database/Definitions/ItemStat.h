#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemStat
    {
        uint32_t ID;
        uint32_t itemStatTypeEnum00;
        uint32_t itemStatTypeEnum01;
        uint32_t itemStatTypeEnum02;
        uint32_t itemStatTypeEnum03;
        uint32_t itemStatTypeEnum04;
        uint32_t itemStatData00;
        uint32_t itemStatData01;
        uint32_t itemStatData02;
        uint32_t itemStatData03;
        uint32_t itemStatData04;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            itemStatTypeEnum00 = file.getUint(recordIndex, i++);
            itemStatTypeEnum01 = file.getUint(recordIndex, i++);
            itemStatTypeEnum02 = file.getUint(recordIndex, i++);
            itemStatTypeEnum03 = file.getUint(recordIndex, i++);
            itemStatTypeEnum04 = file.getUint(recordIndex, i++);
            itemStatData00 = file.getUint(recordIndex, i++);
            itemStatData01 = file.getUint(recordIndex, i++);
            itemStatData02 = file.getUint(recordIndex, i++);
            itemStatData03 = file.getUint(recordIndex, i++);
            itemStatData04 = file.getUint(recordIndex, i++);
        }
    };
}
