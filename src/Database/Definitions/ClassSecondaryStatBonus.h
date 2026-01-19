#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ClassSecondaryStatBonus
    {
        uint32_t ID;
        uint32_t unitProperty2IdSecondaryStat[3];
        float modifier[3];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                unitProperty2IdSecondaryStat[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                modifier[j] = file.getFloat(recordIndex, i++);
        }
    };
}
