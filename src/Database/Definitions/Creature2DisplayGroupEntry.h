#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2DisplayGroupEntry
    {
        uint32_t ID;
        uint32_t creature2DisplayGroupId;
        uint32_t creature2DisplayInfoId;
        uint32_t weight;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2DisplayGroupId = file.getUint(recordIndex, i++);
            creature2DisplayInfoId = file.getUint(recordIndex, i++);
            weight = file.getUint(recordIndex, i++);
        }
    };
}
