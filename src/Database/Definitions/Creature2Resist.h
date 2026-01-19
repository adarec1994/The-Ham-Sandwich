#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2Resist
    {
        uint32_t ID;
        float resistPhysicalMultiplier;
        float resistTechMultiplier;
        float resistMagicMultiplier;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            resistPhysicalMultiplier = file.getFloat(recordIndex, i++);
            resistTechMultiplier = file.getFloat(recordIndex, i++);
            resistMagicMultiplier = file.getFloat(recordIndex, i++);
        }
    };
}
