#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemBudget
    {
        uint32_t ID;
        float budget00;
        float budget01;
        float budget02;
        float budget03;
        float budget04;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            budget00 = file.getFloat(recordIndex, i++);
            budget01 = file.getFloat(recordIndex, i++);
            budget02 = file.getFloat(recordIndex, i++);
            budget03 = file.getFloat(recordIndex, i++);
            budget04 = file.getFloat(recordIndex, i++);
        }
    };
}
