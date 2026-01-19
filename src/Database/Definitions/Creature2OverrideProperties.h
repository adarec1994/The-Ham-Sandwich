#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2OverrideProperties
    {
        uint32_t ID;
        uint32_t creature2Id;
        uint32_t unitPropertyIndex;
        float unitPropertyValue;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2Id = file.getUint(recordIndex, i++);
            unitPropertyIndex = file.getUint(recordIndex, i++);
            unitPropertyValue = file.getFloat(recordIndex, i++);
        }
    };
}
