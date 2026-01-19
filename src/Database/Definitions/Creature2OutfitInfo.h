#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2OutfitInfo
    {
        uint32_t ID;
        uint32_t itemDisplayId[6];
        uint32_t itemColorSetId[6];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                itemDisplayId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                itemColorSetId[j] = file.getUint(recordIndex, i++);
        }
    };
}
