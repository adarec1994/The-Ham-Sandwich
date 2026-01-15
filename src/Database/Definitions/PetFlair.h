#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PetFlair
    {
        uint32_t ID;
        uint32_t unlockBitIndex00;
        uint32_t unlockBitIndex01;
        uint32_t type;
        uint32_t spell4Id;
        uint32_t localizedTextIdTooltip;
        uint32_t itemDisplayId00;
        uint32_t itemDisplayId01;
        uint32_t prerequisiteId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            unlockBitIndex00 = f.getUint(row, i++);
            unlockBitIndex01 = f.getUint(row, i++);
            type = f.getUint(row, i++);
            spell4Id = f.getUint(row, i++);
            localizedTextIdTooltip = f.getUint(row, i++);
            itemDisplayId00 = f.getUint(row, i++);
            itemDisplayId01 = f.getUint(row, i++);
            prerequisiteId = f.getUint(row, i++);
        }
    };
}
