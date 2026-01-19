#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemSetBonus
    {
        uint32_t ID;
        uint32_t requiredPower;
        uint32_t unitProperty2Id;
        float scalar;
        float offset;
        uint32_t spell4Id;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            requiredPower = file.getUint(recordIndex, i++);
            unitProperty2Id = file.getUint(recordIndex, i++);
            scalar = file.getFloat(recordIndex, i++);
            offset = file.getFloat(recordIndex, i++);
            spell4Id = file.getUint(recordIndex, i++);
        }
    };
}
