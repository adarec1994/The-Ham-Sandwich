#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventObjectiveBombDeployment
    {
        uint32_t ID;
        uint32_t creature2IdBomb;
        uint32_t spell4IdCarrying;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            creature2IdBomb = f.getUint(row, i++);
            spell4IdCarrying = f.getUint(row, i++);
        }
    };
}
