#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct LootPinataInfo
    {
        uint32_t ID;
        uint32_t item2Id;
        uint32_t item2TypeId;
        uint32_t item2CategoryId;
        uint32_t item2FamilyId;
        uint32_t virtualItemId;
        uint32_t accountCurrencyTypeId;
        uint32_t creature2IdChest;
        float mass;
        uint32_t soundEventId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            item2Id = file.getUint(recordIndex, i++);
            item2TypeId = file.getUint(recordIndex, i++);
            item2CategoryId = file.getUint(recordIndex, i++);
            item2FamilyId = file.getUint(recordIndex, i++);
            virtualItemId = file.getUint(recordIndex, i++);
            accountCurrencyTypeId = file.getUint(recordIndex, i++);
            creature2IdChest = file.getUint(recordIndex, i++);
            mass = file.getFloat(recordIndex, i++);
            soundEventId = file.getUint(recordIndex, i++);
        }
    };
}
