#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct DistanceDamageModifier
    {
        uint32_t ID;
        float distancePercent00;
        float distancePercent01;
        float distancePercent02;
        float distancePercent03;
        float distancePercent04;
        float damageModifier00;
        float damageModifier01;
        float damageModifier02;
        float damageModifier03;
        float damageModifier04;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            distancePercent00 = file.getFloat(recordIndex, i++);
            distancePercent01 = file.getFloat(recordIndex, i++);
            distancePercent02 = file.getFloat(recordIndex, i++);
            distancePercent03 = file.getFloat(recordIndex, i++);
            distancePercent04 = file.getFloat(recordIndex, i++);
            damageModifier00 = file.getFloat(recordIndex, i++);
            damageModifier01 = file.getFloat(recordIndex, i++);
            damageModifier02 = file.getFloat(recordIndex, i++);
            damageModifier03 = file.getFloat(recordIndex, i++);
            damageModifier04 = file.getFloat(recordIndex, i++);
        }
    };
}
