#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Quest2Difficulty
    {
        uint32_t ID;
        float xpMultiplier;
        float cashRewardMultiplier;
        float repRewardMultiplier;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            xpMultiplier = f.getFloat(row, i++);
            cashRewardMultiplier = f.getFloat(row, i++);
            repRewardMultiplier = f.getFloat(row, i++);
        }
    };
}
