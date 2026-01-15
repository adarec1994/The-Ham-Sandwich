#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PeriodicQuestSet
    {
        uint32_t ID;
        uint32_t periodicQuestSetCategoryId;
        uint32_t periodicGroupsOffered;
        uint32_t weight;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            periodicQuestSetCategoryId = f.getUint(row, i++);
            periodicGroupsOffered = f.getUint(row, i++);
            weight = f.getUint(row, i++);
        }
    };
}
