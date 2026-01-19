#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CreatureLevel
    {
        uint32_t ID;
        float unitPropertyValue[100];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            for (int j = 0; j < 100; ++j)
                unitPropertyValue[j] = file.getFloat(recordIndex, i++);
        }
    };
}
