#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2OutfitGroupEntry
    {
        uint32_t ID;
        uint32_t creature2OutfitGroupId;
        uint32_t creature2OutfitInfoId;
        uint32_t weight;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2OutfitGroupId = file.getUint(recordIndex, i++);
            creature2OutfitInfoId = file.getUint(recordIndex, i++);
            weight = file.getUint(recordIndex, i++);
        }
    };
}
