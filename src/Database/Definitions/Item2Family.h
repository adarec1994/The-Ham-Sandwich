#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Item2Family
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t flags;
        uint32_t soundEventIdEquip;
        float vendorMultiplier;
        float turninMultiplier;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            soundEventIdEquip = file.getUint(recordIndex, i++);
            vendorMultiplier = file.getFloat(recordIndex, i++);
            turninMultiplier = file.getFloat(recordIndex, i++);
        }
    };
}
