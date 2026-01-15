#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4CCConditions
    {
        uint32_t ID;
        uint32_t ccStateMask;
        uint32_t ccStateFlagsRequired;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            ccStateMask = f.getUint(row, i++);
            ccStateFlagsRequired = f.getUint(row, i++);
        }
    };
}
