#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Item2Type
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t itemSlotId;
        uint32_t soundEventIdLoot;
        uint32_t soundEventIdEquip;
        uint32_t flags;
        float vendorMultiplier;
        float turninMultiplier;
        uint32_t Item2CategoryId;
        float referenceMuzzleX;
        float referenceMuzzleY;
        float referenceMuzzleZ;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            itemSlotId = file.getUint(recordIndex, i++);
            soundEventIdLoot = file.getUint(recordIndex, i++);
            soundEventIdEquip = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            vendorMultiplier = file.getFloat(recordIndex, i++);
            turninMultiplier = file.getFloat(recordIndex, i++);
            Item2CategoryId = file.getUint(recordIndex, i++);
            referenceMuzzleX = file.getFloat(recordIndex, i++);
            referenceMuzzleY = file.getFloat(recordIndex, i++);
            referenceMuzzleZ = file.getFloat(recordIndex, i++);
        }
    };
}
