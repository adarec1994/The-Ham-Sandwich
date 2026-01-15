#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4Conditions
    {
        uint32_t ID;
        uint32_t conditionMask;
        uint32_t conditionValue;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            conditionMask = f.getUint(row, i++);
            conditionValue = f.getUint(row, i++);
        }
    };
}
