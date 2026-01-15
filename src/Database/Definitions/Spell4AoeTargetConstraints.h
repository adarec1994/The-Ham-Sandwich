#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4AoeTargetConstraints
    {
        uint32_t ID;
        float angle;
        uint32_t targetCount;
        float minRange;
        float maxRange;
        uint32_t targetSelection;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            angle = f.getFloat(row, i++);
            targetCount = f.getUint(row, i++);
            minRange = f.getFloat(row, i++);
            maxRange = f.getFloat(row, i++);
            targetSelection = f.getUint(row, i++);
        }
    };
}
