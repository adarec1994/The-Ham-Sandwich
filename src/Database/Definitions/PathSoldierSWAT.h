#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSoldierSWAT
    {
        uint32_t ID;
        uint32_t creature2Id;
        uint32_t targetGroupId;
        uint32_t count;
        uint32_t virtualItemIdDisplay;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            creature2Id = f.getUint(row, i++);
            targetGroupId = f.getUint(row, i++);
            count = f.getUint(row, i++);
            virtualItemIdDisplay = f.getUint(row, i++);
        }
    };
}
