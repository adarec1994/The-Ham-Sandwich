#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemRuneSlotRandomization
    {
        uint32_t ID;
        uint32_t microchipTypeEnum;
        uint32_t itemRoleFlagBitMask;
        float randomWeight;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            microchipTypeEnum = file.getUint(recordIndex, i++);
            itemRoleFlagBitMask = file.getUint(recordIndex, i++);
            randomWeight = file.getFloat(recordIndex, i++);
        }
    };
}
