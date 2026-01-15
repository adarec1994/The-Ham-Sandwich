#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PeriodicQuestSetCategory
    {
        uint32_t ID;
        uint32_t periodicSetsOffered;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            periodicSetsOffered = f.getUint(row, i++);
        }
    };
}
